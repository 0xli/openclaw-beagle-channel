/**
 * Beagle Chat (Elastos Carrier) Channel Provider for OpenClaw
 * 
 * This module provides integration between OpenClaw and Beagle Chat
 * via the Elastos Carrier protocol.
 */

export interface BeagleConfig {
  /** Sidecar daemon base URL (e.g., 'http://localhost:8765') */
  sidecarUrl: string;
  
  /** Authentication token for sidecar API */
  authToken?: string;
  
  /** Enable debug logging */
  debug?: boolean;
  
  /** Reconnect interval in milliseconds */
  reconnectInterval?: number;
}

export interface BeagleMessage {
  /** Unique message ID */
  messageId: string;
  
  /** Sender peer ID (Carrier address) */
  peerId: string;
  
  /** Message text content */
  text: string;
  
  /** Timestamp (Unix milliseconds) */
  timestamp: number;
  
  /** Optional: Chat/conversation ID */
  chatId?: string;
}

export interface SendMessageRequest {
  /** Target peer ID (Carrier address) */
  peerId: string;
  
  /** Message text to send */
  text: string;
}

export interface SendMessageResponse {
  /** Generated message ID */
  messageId: string;
  
  /** Timestamp when sent */
  timestamp: number;
  
  /** Success status */
  success: boolean;
}

export interface InboundMessageHandler {
  (message: BeagleMessage): void | Promise<void>;
}
