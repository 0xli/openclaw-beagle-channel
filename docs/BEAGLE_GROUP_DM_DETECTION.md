# Beagle Group vs DM Detection (CarrierGroup-Compatible)

## Why this exists

CarrierGroup forwards group traffic to agent friends using the **group peer userid** as message sender.
With the updated protocol, CarrierGroup now includes structured metadata in a `CGP1` envelope.

So Beagle sidecar/channel must infer:

- conversation scope: `group` vs `direct`
- original sender userid/display name
- routing target for replies

## Current CarrierGroup forwarding shape

Observed group format:

- Prefix: `CGP1 `
- JSON: `type="carrier_group_message"`, `chat_type="group"`
- Includes:
  - `group.userid`, `group.address`, `group.nickname`
  - `origin.userid`, `origin.nickname`
  - `origin.user_info`, `origin.friend_info`
  - `message.text`, `message.timestamp`

## Detection rules in channel

Given inbound sidecar event (`peer`, `text`):

1. If text starts with `CGP1 ` and JSON parses.
2. Require:
   - `type == "carrier_group_message"`
   - `group.address` present
   - `origin.userid` present
   - `message.text` present
3. If all pass, treat as group.
4. Otherwise treat as direct chat.

Security note:

- This is format-based classification.
- Without an allowlist of trusted group peers/addresses (or signatures), a DM sender could forge payload shape.

## Routing semantics (aligned with Telegram/Discord concept)

- Conversation id for group mode: `group.userid` (fallback to inbound Carrier `peer`)
- Sender identity in group mode: `origin.userid` + `origin.nickname`
- Reply target in both modes: `peer` from inbound event
  - group mode replies go to group peer, then CarrierGroup fans out to members

When replying to group context, channel sends:

- Prefix: `CGR1 `
- JSON: `type="carrier_group_reply"`, `chat_type="group"`, `group.address`, `message.text`

## Config

No mandatory config is required for group detection.

Optional per-account trust config (recommended in production):

```json
{
  "channels": {
    "beagle": {
      "accounts": {
        "default": {
          "trustedGroupPeers": ["aHzsSg...6377", "ehmqad...LCzL"],
          "trustedGroupAddresses": ["EHta...6377", "t4Jr...LCzL"],
          "requireTrustedGroup": false
        }
      }
    }
  }
}
```
