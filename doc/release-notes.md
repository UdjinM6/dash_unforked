Dash Core version v19.0.0
=========================

Release is now available from:

  <https://www.dash.org/downloads/#wallets>

This is a new major version release, bringing new features, various bugfixes
and other improvements.

This release is mandatory for all nodes.

Please report bugs using the issue tracker at github:

  <https://github.com/dashpay/dash/issues>


Upgrading and downgrading
=========================

How to Upgrade
--------------

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over /Applications/Dash-Qt (on Mac) or
dashd/dash-qt (on Linux). If you upgrade after DIP0003 activation and you were
using version < 0.13 you will have to reindex (start with -reindex-chainstate
or -reindex) to make sure your wallet has all the new data synced. Upgrading
from version 0.13 should not require any additional actions.

When upgrading from a version prior to 18.0.1, the
first startup of Dash Core will run a migration process which can take anywhere
from a few minutes to thirty minutes to finish. After the migration, a
downgrade to an older version is only possible with a reindex
(or reindex-chainstate).

Downgrade warning
-----------------

### Downgrade to a version < v19.0.0

Downgrading to a version older than v19.0.0 is not supported due to changes in
the evodb database. If you need to use an older version, you must
either reindex or re-sync the whole chain.

Notable changes
===============

High-Performance Masternodes
----------------------------
In preparation for the release of Dash Platform to mainnet, a new masternode
type has been added - High-Performance Masternodes (HPMNs). HPMNs will be
responsible for hosting Dash Platform services (once they are on mainnet) and
existing features like ChainLocks and InstantSend.

Activation of the DashCore v19.0 hard fork will enable registration of the new
4000 DASH collateral masternodes. Until Dash Platform is released to mainnet,
HPMNs will provide the same services as regular masternodes with one small
exception. Regular masternodes will no longer participate in the Platform-
specific LLMQ after the hard fork since they will not be responsible for
hosting Dash Platform.

Note: In DashCore v19.0.0 the relative rewards and voting power are equivalent
between regular and HPMNs. Masternodes effectively receive one payout and one
governance vote per 1000 DASH collateral. So, there is no difference in reward
amount for running four regular masternodes or one HPMN. In v19.0, HPMNs simply
receive payments in four consecutive blocks when they are selected for payout.
Some frequently asked questions may be found at https://www.dash.org/hpmn-faq/.

BLS scheme migration
--------------------
DashCore’s default BLS scheme has been changed to better align with standards
and improve security. This is the continuation of an effort that started in a
previous release. Upon activation of DashCore v19’s hard fork, the network will
transition to using the BLS basic scheme rather than the previously used
"legacy” scheme. As a result, BLS public keys and signatures in network
messages, special transactions, etc. will be serialized using the new scheme.

List of affected messages: `clsig`, `dsq`, `dstx`, `govobj`, `govobjvote`,
`isdlock`, `mnauth`, `qcomplaint`, `qcontrib`, `qfcommit`, `qjustify`,
`qpcommit`, `qrinfo`, `qsigrec`, `qsigshare`.

### `qfcommit`

Once the v19 hard fork is activated, `quorumPublicKey` will be serialised using the basic BLS scheme.
To support syncing of older blocks containing the transactions using the legacy BLS scheme, the `version` field indicates which scheme to use for serialisation of `quorumPublicKey`.

| Version | Version Description                                    | Includes `quorumIndex` field |
|---------|--------------------------------------------------------|------------------------------|
| 1       | Non-rotated qfcommit serialised using legacy BLS scheme | No                           |
| 2       | Rotated qfcommit serialised using legacy BLS scheme     | Yes                          |
| 3       | Non-rotated qfcommit serialised using basic BLS scheme  | No                           |
| 4       | Rotated qfcommit serialised using basic BLS scheme      | Yes                          |

### `MNLISTDIFF` P2P message

Starting with protocol version 70225, the following field is added to the `MNLISTDIFF` message between `cbTx` and `deletedQuorumsCount`.

| Field               | Type | Size | Description                       |
|---------------------| ---- | ---- |-----------------------------------|
| version             | uint16_t | 2 | Version of the `MNLISTDIFF` reply |

The `version` field indicates which BLS scheme is used to serialise the `pubKeyOperator` field for all SML entries of `mnList`.

| Version | Version Description                                       |
|---------|-----------------------------------------------------------|
| 1       | Serialisation of `pubKeyOperator` using legacy BLS scheme |
| 2       | Serialisation of `pubKeyOperator` using basic BLS scheme  |

### `ProTx` txs family

`proregtx` and `proupregtx` will support a new `version` value:

| Version | Version Description                                       |
|---------|-----------------------------------------------------------|
| 1       | Serialisation of `pubKeyOperator` using legacy BLS scheme |
| 2       | Serialisation of `pubKeyOperator` using basic BLS scheme  |

`proupservtx` and `prouprevtx` will support a new `version` value:

| Version | Version Description                            |
|---------|------------------------------------------------|
| 1       | Serialisation of `sig` using legacy BLS scheme |
| 2       | Serialisation of `sig` using basic BLS scheme  |

### `MNHFTx`

`MNHFTx` will support a new `version` value:

| Version | Version Description                            |
|---------|------------------------------------------------|
| 1       | Serialisation of `sig` using legacy BLS scheme |
| 2       | Serialisation of `sig` using basic BLS scheme  |


Wallet improvements
-------------------
Several wallet modifications have been included in this release. Dash Core will
no longer automatically create new wallets on startup. It will load existing
wallets specified by `-wallet` options on the command line or in `dash.conf` or
`settings.json` files. And by default it will also load a top-level unnamed
("") wallet. However, if specified wallets don't exist, Dash Core will now just
log warnings instead of creating new wallets with new keys and addresses like
previous releases did.

New wallets can be created through the GUI (which has a more prominent create
wallet option), through the `dash-wallet create` command, or the `createwallet`
RPC.

Transactions that appear “stuck” will now be re-sent after one hour on average
rather than the extended 24 hour delay that was previously in place. The lack
of congestion on the Dash network allowed reducing the timeframe without
performance concerns.

### TODO: sqlite support. is there a way to control which db is used to create new wallet?

CoinJoin update
---------------
A minor update in several CoinJoin-related network messages improves support
for mixing from SPV clients. These changes make it easier for SPV clients to
participate in the CoinJoin process by using masternode information they can
readily obtain and verify via [DIP-0004](https://github.com/dashpay/dips/blob/master/dip-0004.md).

Reject message removal (BIP61)
------------------------------
The `reject` network message was previously deprecated, but has been removed
entirely in DashCore v19.0.0. In the past some clients used these messages to
detect things like accidental double-spends among other things. This change was
backported from Bitcoin which removed support for reject messages several
versions ago.

New quorum types
----------------
`LLMQ_TEST_PLATFORM` (regtest only) and `LLMQ_DEVNET_PLATFORM` (devnet only)
quorum types were added. Please see [DIP-0006](https://github.com/dashpay/dips/blob/master/dip-0006/llmq-types.md) for more info.

Remote Procedure Call (RPC) Changes
-----------------------------------
DashCore v19.0 introduces eight new Dash-specific RPC commands. The new RPCs
are all related to masternode management:
- `protx register_legacy`
- `protx register_fund_legacy`
- `protx register_prepare_legacy`
- `protx update_registrar_legacy`
- `protx register_hpmn`
- `protx register_fund_hpmn`
- `protx register_prepare_hpmn`
- `protx update_service_hpmn`

These HPMN RPCs correspond to the standard masternode RPCs but have the
following additional mandatory arguments:
- `platformNodeID`: Platform P2P node ID, derived from P2P public key.
- `platformP2PPort`: TCP port of Dash Platform peer-to-peer communication between nodes (network byte order).
- `platformHTTPPort`: TCP port of Platform HTTP/API interface (network byte order).

Notes:
- `platformNodeID` must be unique across the network.
- `platformP2PPort`, `platformHTTPPort` and the Core port must be distinct.

Other new RPCs:
- `analyzepsbt`
- `cleardiscouraged`: clears all the already discouraged peers.
- `upgradewallet`

Updated RPCs:
- `bls generate` and `bls fromsecret`: Both RPCs accept an incoming optional boolean argument `legacy` that enforces the use of legacy BLS scheme for the serialisation of the reply even if v19 is active.
- `bls generate` and `bls fromsecret`: The new `scheme` field will be returned indicating which scheme was used to serialise the public key. Valid returned values are `legacy` and`basic`.
- `getaddressinfo` returns an array of addresses (`addresses`) associated with the known redeemscript (only if `script` is `multisig`)
- `gobject getcurrentvotes`: reply is enriched by adding the vote weight at the end of each line
- `masternode` mode `count` now returns a detailed summary of total and enabled masternodes per type.
- `masternode` mode `status` now returns the type of the masternode.
- `masternodelist`: New mode `recent` was added in order to hide banned masternodes for more than one `SuperblockCycle`. If the mode `recent` is used, then the reply mode is JSON (can be additionally filtered).
- `quorum info`: The new `previousConsecutiveDKGFailures` field will be returned for rotated LLMQs. This field will hold the number of previous consecutive DKG failures for the corresponding quorumIndex before the currently active one. Note: If no previous commitments were found then 0 will be returned for `previousConsecutiveDKGFailures`.
- `sendrawtransaction` and `testmempoolaccept` no longer have `allowhighfees` option
- `utxoupdatepsbt`

No RPCs have been removed in this release.

Command-line Options
--------------------
A number of command-line option changes were made related to testing and
removal of BIP61 support.

New cmd-line options:
- `llmqplatform` (devnet only)
- `unsafesqlitesync`

Removed cmd-line options:
- `enablebip61`
- `upgradewallet`

Changes in existing cmd-line options:
- `llmqinstantsend` and `llmqinstantsenddip0024` can be used in regtest now

Please check `Help -> Command-line options` in Qt wallet or `dashd --help` for
more information.

Changes in GUI
--------------
`Masternodes` tab shows masternode type column now.
`Upgrade wallet` option was removed from `Wallet Repair` window.

Backports from Bitcoin Core
---------------------------
We have backported hundreds of items from Bitcoin v0.19-v0.21 which are
included in DashCore v19.0. In addition, select items have been backported from
Bitcoin v22.0 and v23.0.

Miscellaneous
-------------
A lot of refactoring, code cleanups and other small fixes were done in this release.

v19.0.0 Change log
==================

See detailed [set of changes](https://github.com/dashpay/dash/compare/v18.2.2...dashpay:v19.0.0).

Credits
=======

Thanks to everyone who directly contributed to this release:

- Kittywhiskers Van Gogh (kittywhiskers)
- Konstantin Akimov (knst)
- Odysseas Gabrielides (ogabrielides)
- Oleg Girko (OlegGirko)
- PastaPastaPasta
- thephez
- UdjinM6
- Vijay Das Manikpuri (vijaydasmp)

As well as everyone that submitted issues, reviewed pull requests, helped debug the release candidates, and write DIPs that were implemented in this release.

Older releases
==============

Dash was previously known as Darkcoin.

Darkcoin tree 0.8.x was a fork of Litecoin tree 0.8, original name was XCoin
which was first released on Jan/18/2014.

Darkcoin tree 0.9.x was the open source implementation of masternodes based on
the 0.8.x tree and was first released on Mar/13/2014.

Darkcoin tree 0.10.x used to be the closed source implementation of Darksend
which was released open source on Sep/25/2014.

Dash Core tree 0.11.x was a fork of Bitcoin Core tree 0.9,
Darkcoin was rebranded to Dash.

Dash Core tree 0.12.0.x was a fork of Bitcoin Core tree 0.10.

Dash Core tree 0.12.1.x was a fork of Bitcoin Core tree 0.12.

These release are considered obsolete. Old release notes can be found here:

- [v18.2.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-18.2.2.md) released Mar/21/2023
- [v18.2.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-18.2.1.md) released Jan/17/2023
- [v18.2.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-18.2.0.md) released Jan/01/2023
- [v18.1.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-18.1.1.md) released January/08/2023
- [v18.1.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-18.1.0.md) released October/09/2022
- [v18.0.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-18.0.2.md) released October/09/2022
- [v18.0.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-18.0.1.md) released August/17/2022
- [v0.17.0.3](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.17.0.3.md) released June/07/2021
- [v0.17.0.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.17.0.2.md) released May/19/2021
- [v0.16.1.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.16.1.1.md) released November/17/2020
- [v0.16.1.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.16.1.0.md) released November/14/2020
- [v0.16.0.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.16.0.1.md) released September/30/2020
- [v0.15.0.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.15.0.0.md) released Febrary/18/2020
- [v0.14.0.5](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.14.0.5.md) released December/08/2019
- [v0.14.0.4](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.14.0.4.md) released November/22/2019
- [v0.14.0.3](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.14.0.3.md) released August/15/2019
- [v0.14.0.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.14.0.2.md) released July/4/2019
- [v0.14.0.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.14.0.1.md) released May/31/2019
- [v0.14.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.14.0.md) released May/22/2019
- [v0.13.3](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.13.3.md) released Apr/04/2019
- [v0.13.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.13.2.md) released Mar/15/2019
- [v0.13.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.13.1.md) released Feb/9/2019
- [v0.13.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.13.0.md) released Jan/14/2019
- [v0.12.3.4](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.12.3.4.md) released Dec/14/2018
- [v0.12.3.3](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.12.3.3.md) released Sep/19/2018
- [v0.12.3.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.12.3.2.md) released Jul/09/2018
- [v0.12.3.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.12.3.1.md) released Jul/03/2018
- [v0.12.2.3](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.12.2.3.md) released Jan/12/2018
- [v0.12.2.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.12.2.2.md) released Dec/17/2017
- [v0.12.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.12.2.md) released Nov/08/2017
- [v0.12.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.12.1.md) released Feb/06/2017
- [v0.12.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.12.0.md) released Aug/15/2015
- [v0.11.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.11.2.md) released Mar/04/2015
- [v0.11.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.11.1.md) released Feb/10/2015
- [v0.11.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.11.0.md) released Jan/15/2015
- [v0.10.x](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.10.0.md) released Sep/25/2014
- [v0.9.x](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.9.0.md) released Mar/13/2014
