# API Reference

## Channel Plugin API

### BeagleChannelPlugin

The main plugin class that implements the OpenClaw channel interface.

#### Constructor

```typescript
new BeagleChannelPlugin(sidecarUrl?: string)
```

**Parameters:**
- `sidecarUrl` (optional): URL of the Beagle sidecar API. Defaults to `http://localhost:8080`

#### Properties

- `name: string` - Channel name ("beagle")
- `capabilities: ChannelCapabilities` - Channel capabilities

#### Methods

##### initialize()

Initialize the plugin and connect to the sidecar WebSocket.

```typescript
await plugin.initialize(): Promise<void>
```

##### sendText(context)

Send a text message.

```typescript
await plugin.sendText(context: ChannelSendTextContext): Promise<void>
```

**Parameters:**
- `context.channelId: string` - Channel ID
- `context.peer: string` - Recipient peer ID
- `context.text: string` - Message text

##### sendMedia(context)

Send a media message (image, document, audio, video, voice).

```typescript
await plugin.sendMedia(context: ChannelSendMediaContext): Promise<void>
```

**Parameters:**
- `context.channelId: string` - Channel ID
- `context.peer: string` - Recipient peer ID
- `context.mediaType: MediaType` - Type of media
- `context.text?: string` - Optional caption
- `context.mediaPath?: string` - Local file path (one of mediaPath or mediaUrl required)
- `context.mediaUrl?: string` - Remote URL (one of mediaPath or mediaUrl required)
- `context.filename?: string` - Optional filename
- `context.mimeType?: string` - Optional MIME type

##### onMessage(handler)

Register a handler for inbound messages.

```typescript
plugin.onMessage(handler: (message: InboundMessage) => void): void
```

**Parameters:**
- `handler` - Callback function to handle inbound messages

##### cleanup()

Clean up resources and disconnect from sidecar.

```typescript
await plugin.cleanup(): Promise<void>
```

## Type Definitions

### MediaType

```typescript
enum MediaType {
  Image = 'image',
  Audio = 'audio',
  Video = 'video',
  Document = 'document',
  Voice = 'voice'
}
```

### MediaCapabilities

```typescript
interface MediaCapabilities {
  sendImage?: boolean;
  receiveImage?: boolean;
  sendDocument?: boolean;
  receiveDocument?: boolean;
  sendAudio?: boolean;
  receiveAudio?: boolean;
  sendVideo?: boolean;
  receiveVideo?: boolean;
  sendVoice?: boolean;
  receiveVoice?: boolean;
}
```

### ChannelCapabilities

```typescript
interface ChannelCapabilities {
  chatTypes: string[];  // e.g., ["direct"]
  media?: MediaCapabilities;
}
```

### ChannelSendTextContext

```typescript
interface ChannelSendTextContext {
  channelId: string;
  peer: string;
  text: string;
}
```

### ChannelSendMediaContext

```typescript
interface ChannelSendMediaContext {
  channelId: string;
  peer: string;
  text?: string;         // Optional caption
  mediaPath?: string;    // Local file path
  mediaUrl?: string;     // Remote URL
  mediaType: MediaType;
  filename?: string;
  mimeType?: string;
}
```

### InboundMessage

```typescript
interface InboundMessage {
  peer: string;          // Sender ID
  text?: string;         // Message text or caption
  mediaPath?: string;    // Local path to downloaded media
  mediaUrl?: string;     // URL to media
  mediaType?: MediaType; // Type of media
  filename?: string;     // Original filename
  size?: number;         // File size in bytes
  timestamp: number;     // Unix timestamp
  messageId: string;     // Unique message ID
  transcript?: string;   // For audio/voice messages
}
```

## Sidecar Client API

### BeagleSidecar

Low-level client for communicating with the Beagle sidecar API.

#### Constructor

```typescript
new BeagleSidecar(baseUrl: string)
```

#### Methods

##### connect()

Connect to the sidecar WebSocket.

```typescript
await sidecar.connect(): Promise<void>
```

##### disconnect()

Disconnect from the sidecar.

```typescript
await sidecar.disconnect(): Promise<void>
```

##### onMessage(handler)

Register a handler for inbound messages.

```typescript
sidecar.onMessage(handler: (message: InboundMessage) => void): void
```

##### sendText(request)

Send a text message via the sidecar.

```typescript
await sidecar.sendText(request: SendTextRequest): Promise<void>
```

**SendTextRequest:**
```typescript
interface SendTextRequest {
  peer: string;
  text: string;
}
```

##### sendMedia(request)

Send a media message via the sidecar.

```typescript
await sidecar.sendMedia(request: SendMediaRequest): Promise<void>
```

**SendMediaRequest:**
```typescript
interface SendMediaRequest {
  peer: string;
  path?: string;
  url?: string;
  mime?: string;
  filename?: string;
  caption?: string;
}
```

## Example Usage

See [examples/usage.ts](examples/usage.ts) for complete examples.

### Basic Text Message

```typescript
import { BeagleChannelPlugin } from 'openclaw-beagle-channel';

const plugin = new BeagleChannelPlugin('http://localhost:8080');
await plugin.initialize();

await plugin.sendText({
  channelId: 'beagle',
  peer: 'user-123',
  text: 'Hello!'
});
```

### Send Image with Caption

```typescript
import { BeagleChannelPlugin, MediaType } from 'openclaw-beagle-channel';

const plugin = new BeagleChannelPlugin('http://localhost:8080');
await plugin.initialize();

await plugin.sendMedia({
  channelId: 'beagle',
  peer: 'user-123',
  mediaPath: '/path/to/image.jpg',
  mediaType: MediaType.Image,
  text: 'Check this out!'
});
```

### Receive Messages

```typescript
import { BeagleChannelPlugin } from 'openclaw-beagle-channel';

const plugin = new BeagleChannelPlugin('http://localhost:8080');
await plugin.initialize();

plugin.onMessage((message) => {
  console.log('From:', message.peer);
  console.log('Text:', message.text);
  
  if (message.mediaType) {
    console.log('Media:', message.mediaType);
    console.log('URL/Path:', message.mediaUrl || message.mediaPath);
  }
});
```
