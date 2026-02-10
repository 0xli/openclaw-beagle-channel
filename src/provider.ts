import { BeagleChannelClient } from './client';
import { BeagleConfig, BeagleMessage, SendMessageRequest } from './types';

/**
 * OpenClaw Channel Provider Interface
 * 
 * This class implements the OpenClaw channel provider pattern for Beagle Chat.
 * It handles both outbound (sending) and inbound (receiving) messages.
 */
export class BeagleChannelProvider {
  private client: BeagleChannelClient;
  private channelId = 'beagle';
  private displayName = 'Beagle Chat';

  constructor(config: BeagleConfig) {
    this.client = new BeagleChannelClient(config);
  }

  /**
   * Get the channel ID (technical identifier)
   */
  getChannelId(): string {
    return this.channelId;
  }

  /**
   * Get the display name (user-facing)
   */
  getDisplayName(): string {
    return this.displayName;
  }

  /**
   * Initialize the channel provider
   */
  async initialize(): Promise<void> {
    // Verify sidecar is running
    const healthy = await this.client.healthCheck();
    if (!healthy) {
      throw new Error('Beagle Chat sidecar is not running or not healthy');
    }

    // Connect to event stream
    await this.client.connect();
  }

  /**
   * Send a text message to a peer
   * 
   * This is the main outbound method called by OpenClaw.
   * 
   * @param peerId - Target peer ID (Carrier address)
   * @param text - Message text to send
   * @returns Message ID and timestamp
   */
  async sendText(peerId: string, text: string): Promise<{ messageId: string; timestamp: number }> {
    const request: SendMessageRequest = {
      peerId,
      text,
    };

    const response = await this.client.sendMessage(request);
    
    if (!response.success) {
      throw new Error('Failed to send message to Beagle Chat');
    }

    return {
      messageId: response.messageId,
      timestamp: response.timestamp,
    };
  }

  /**
   * Register a handler for incoming messages
   * 
   * OpenClaw will call this to receive inbound messages from Beagle Chat.
   * 
   * @param handler - Callback function to handle incoming messages
   */
  onIncomingMessage(handler: (message: BeagleMessage) => void | Promise<void>): void {
    this.client.onMessage(handler);
  }

  /**
   * Graceful shutdown
   */
  async shutdown(): Promise<void> {
    await this.client.disconnect();
  }

  /**
   * Get connection status
   */
  isConnected(): boolean {
    return this.client.isConnected();
  }
}
