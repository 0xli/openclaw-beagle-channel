/**
 * Beagle Chat (Elastos Carrier) Channel Provider for OpenClaw
 * 
 * Main entry point for the OpenClaw Beagle Chat integration.
 */

export { BeagleChannelProvider } from './provider';
export { BeagleChannelClient } from './client';
export type {
  BeagleConfig,
  BeagleMessage,
  SendMessageRequest,
  SendMessageResponse,
  InboundMessageHandler,
} from './types';

/**
 * Example usage:
 * 
 * ```typescript
 * import { BeagleChannelProvider } from 'openclaw-beagle-channel';
 * 
 * const provider = new BeagleChannelProvider({
 *   sidecarUrl: 'http://localhost:8765',
 *   authToken: 'your-secure-token',
 *   debug: true,
 * });
 * 
 * await provider.initialize();
 * 
 * // Send a message
 * await provider.sendText('peer-id-123', 'Hello from OpenClaw!');
 * 
 * // Receive messages
 * provider.onIncomingMessage(async (message) => {
 *   console.log('New message:', message.text, 'from', message.peerId);
 * });
 * ```
 */
