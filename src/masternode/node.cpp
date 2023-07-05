// Copyright (c) 2014-2022 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <masternode/node.h>

#include <evo/deterministicmns.h>

#include <chainparams.h>
#include <net.h>
#include <netbase.h>
#include <protocol.h>
#include <validation.h>
#include <warnings.h>

// Keep track of the active Masternode
RecursiveMutex activeMasternodeInfoCs;
CActiveMasternodeInfo activeMasternodeInfo GUARDED_BY(activeMasternodeInfoCs);
std::unique_ptr<CActiveMasternodeManager> activeMasternodeManager;

std::string CActiveMasternodeManager::GetStateString() const
{
    switch (state) {
    case MasternodeState::WAITING_FOR_PROTX:
        return "WAITING_FOR_PROTX";
    case MasternodeState::POSE_BANNED:
        return "POSE_BANNED";
    case MasternodeState::REMOVED:
        return "REMOVED";
    case MasternodeState::OPERATOR_KEY_CHANGED:
        return "OPERATOR_KEY_CHANGED";
    case MasternodeState::PROTX_IP_CHANGED:
        return "PROTX_IP_CHANGED";
    case MasternodeState::READY:
        return "READY";
    case MasternodeState::ERROR:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}

std::string CActiveMasternodeManager::GetStatus() const
{
    switch (state) {
    case MasternodeState::WAITING_FOR_PROTX:
        return "Waiting for ProTx to appear on-chain";
    case MasternodeState::POSE_BANNED:
        return "Masternode was PoSe banned";
    case MasternodeState::REMOVED:
        return "Masternode removed from list";
    case MasternodeState::OPERATOR_KEY_CHANGED:
        return "Operator key changed or revoked";
    case MasternodeState::PROTX_IP_CHANGED:
        return "IP address specified in ProTx changed";
    case MasternodeState::READY:
        return "Ready";
    case MasternodeState::ERROR:
        return "Error. " + strError;
    default:
        return "Unknown";
    }
}

void CActiveMasternodeManager::Init(const CBlockIndex* pindex)
{
    LOCK2(cs_main, activeMasternodeInfoCs);

    if (!fMasternodeMode) return;

    if (!CDeterministicMNManager::IsDIP3Enforced(pindex->nHeight)) return;

    // Check that our local network configuration is correct
    if (!fListen && Params().RequireRoutableExternalIP()) {
        // listen option is probably overwritten by something else, no good
        state = MasternodeState::ERROR;
        strError = "Masternode must accept connections from outside. Make sure listen configuration option is not overwritten by some another parameter.";
        LogPrintf("CActiveMasternodeManager::Init -- ERROR: %s\n", strError);
        return;
    }

    if (!GetLocalAddress(activeMasternodeInfo.service)) {
        state = MasternodeState::ERROR;
        return;
    }

    CDeterministicMNList mnList = deterministicMNManager->GetListForBlock(pindex);

    CDeterministicMNCPtr dmn = mnList.GetMNByOperatorKey(*activeMasternodeInfo.blsPubKeyOperator);
    if (!dmn) {
        // MN not appeared on the chain yet
        return;
    }

    if (!mnList.IsMNValid(dmn->proTxHash)) {
        if (mnList.IsMNPoSeBanned(dmn->proTxHash)) {
            state = MasternodeState::POSE_BANNED;
        } else {
            state = MasternodeState::REMOVED;
        }
        return;
    }

    LogPrintf("CActiveMasternodeManager::Init -- proTxHash=%s, proTx=%s\n", dmn->proTxHash.ToString(), dmn->ToString());

    if (activeMasternodeInfo.service != dmn->pdmnState->addr) {
        state = MasternodeState::ERROR;
        strError = "Local address does not match the address from ProTx";
        LogPrintf("CActiveMasternodeManager::Init -- ERROR: %s\n", strError);
        return;
    }

    // Check socket connectivity
    LogPrintf("CActiveMasternodeManager::Init -- Checking inbound connection to '%s'\n", activeMasternodeInfo.service.ToString());
    std::unique_ptr<Sock> sock = CreateSock(activeMasternodeInfo.service);
    if (!sock) {
        state = MasternodeState::ERROR;
        strError = "Could not create socket to connect to " + activeMasternodeInfo.service.ToString();
        LogPrintf("CActiveMasternodeManager::Init -- ERROR: %s\n", strError);
        return;
    }
    bool fConnected = ConnectSocketDirectly(activeMasternodeInfo.service, sock->Get(), nConnectTimeout, true) && IsSelectableSocket(sock->Get());
    sock->Reset();

    if (!fConnected && Params().RequireRoutableExternalIP()) {
        state = MasternodeState::ERROR;
        strError = "Could not connect to " + activeMasternodeInfo.service.ToString();
        LogPrintf("CActiveMasternodeManager::Init -- ERROR: %s\n", strError);
        return;
    }

    activeMasternodeInfo.proTxHash = dmn->proTxHash;
    activeMasternodeInfo.outpoint = dmn->collateralOutpoint;
    activeMasternodeInfo.legacy = dmn->pdmnState->nVersion == CProRegTx::LEGACY_BLS_VERSION;
    state = MasternodeState::READY;
}

void CActiveMasternodeManager::UpdatedBlockTip(const CBlockIndex* pindexNew, const CBlockIndex* pindexFork, bool fInitialDownload)
{
    LOCK2(cs_main, activeMasternodeInfoCs);

    if (!fMasternodeMode) return;

    if (!CDeterministicMNManager::IsDIP3Enforced(pindexNew->nHeight)) return;

    if (state == MasternodeState::READY) {
        auto oldMNList = deterministicMNManager->GetListForBlock(pindexNew->pprev);
        auto newMNList = deterministicMNManager->GetListForBlock(pindexNew);
        if (!newMNList.IsMNValid(activeMasternodeInfo.proTxHash)) {
            // MN disappeared from MN list
            state = MasternodeState::REMOVED;
            activeMasternodeInfo.proTxHash = uint256();
            activeMasternodeInfo.outpoint.SetNull();
            // MN might have reappeared in same block with a new ProTx
            Init(pindexNew);
            return;
        }

        auto oldDmn = oldMNList.GetMN(activeMasternodeInfo.proTxHash);
        auto newDmn = newMNList.GetMN(activeMasternodeInfo.proTxHash);
        if (newDmn->pdmnState->pubKeyOperator != oldDmn->pdmnState->pubKeyOperator) {
            // MN operator key changed or revoked
            state = MasternodeState::OPERATOR_KEY_CHANGED;
            activeMasternodeInfo.proTxHash = uint256();
            activeMasternodeInfo.outpoint.SetNull();
            // MN might have reappeared in same block with a new ProTx
            Init(pindexNew);
            return;
        }

        if (newDmn->pdmnState->addr != oldDmn->pdmnState->addr) {
            // MN IP changed
            state = MasternodeState::PROTX_IP_CHANGED;
            activeMasternodeInfo.proTxHash = uint256();
            activeMasternodeInfo.outpoint.SetNull();
            Init(pindexNew);
            return;
        }
    } else {
        // MN might have (re)appeared with a new ProTx or we've found some peers
        // and figured out our local address
        Init(pindexNew);
    }
}

bool CActiveMasternodeManager::GetLocalAddress(CService& addrRet)
{
    // First try to find whatever our own local address is known internally.
    // Addresses could be specified via externalip or bind option, discovered via UPnP
    // or added by TorController. Use some random dummy IPv4 peer to prefer the one
    // reachable via IPv4.
    CNetAddr addrDummyPeer;
    bool fFoundLocal{false};
    if (LookupHost("8.8.8.8", addrDummyPeer, false)) {
        fFoundLocal = GetLocal(addrRet, &addrDummyPeer) && IsValidNetAddr(addrRet);
    }
    if (!fFoundLocal && !Params().RequireRoutableExternalIP()) {
        if (Lookup("127.0.0.1", addrRet, GetListenPort(), false)) {
            fFoundLocal = true;
        }
    }
    if (!fFoundLocal) {
        bool empty = true;
        // If we have some peers, let's try to find our local address from one of them
        auto service = WITH_LOCK(activeMasternodeInfoCs, return activeMasternodeInfo.service);
        connman.ForEachNodeContinueIf(CConnman::AllNodes, [&](CNode* pnode) {
            empty = false;
            if (pnode->addr.IsIPv4())
                fFoundLocal = GetLocal(service, &pnode->addr) && IsValidNetAddr(service);
            return !fFoundLocal;
        });
        // nothing and no live connections, can't do anything for now
        if (empty) {
            strError = "Can't detect valid external address. Please consider using the externalip configuration option if problem persists. Make sure to use IPv4 address only.";
            LogPrintf("CActiveMasternodeManager::GetLocalAddress -- ERROR: %s\n", strError);
            return false;
        }
    }
    return true;
}

bool CActiveMasternodeManager::IsValidNetAddr(CService addrIn)
{
    // TODO: regtest is fine with any addresses for now,
    // should probably be a bit smarter if one day we start to implement tests for this
    return !Params().RequireRoutableExternalIP() ||
           (addrIn.IsIPv4() && IsReachable(addrIn) && addrIn.IsRoutable());
}
