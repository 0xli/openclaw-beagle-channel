/**
 * Beagle Chat channel plugin types for OpenClaw
 */

/**
 * Configuration options for the Beagle channel
 */
export interface BeagleChannelConfig {
  /**
   * Beagle API endpoint URL
   */
  apiUrl: string;

  /**
   * Authentication token for Beagle API
   */
  authToken: string;

  /**
   * Webhook secret for validating incoming messages
   */
  webhookSecret?: string;

  /**
   * Port for webhook server (if needed)
   */
  webhookPort?: number;

  /**
   * Enable debug logging
   */
  debug?: boolean;
}

/**
 * Beagle user information
 */
export interface BeagleUser {
  /**
   * User ID
   */
  id: string;

  /**
   * User's display name
   */
  name: string;

  /**
   * User's email (optional)
   */
  email?: string;

  /**
   * User's avatar URL (optional)
   */
  avatar?: string;
}

/**
 * Beagle message structure
 */
export interface BeagleMessage {
  /**
   * Message ID
   */
  id: string;

  /**
   * Sender information
   */
  from: BeagleUser;

  /**
   * Recipient information (for DM)
   */
  to?: BeagleUser;

  /**
   * Message text content
   */
  text: string;

  /**
   * Message timestamp
   */
  timestamp: number;

  /**
   * Message type (text, image, file, etc.)
   */
  type?: string;

  /**
   * Attachments (optional)
   */
  attachments?: BeagleAttachment[];
}

/**
 * Beagle message attachment
 */
export interface BeagleAttachment {
  /**
   * Attachment type
   */
  type: 'image' | 'file' | 'link';

  /**
   * Attachment URL
   */
  url: string;

  /**
   * Attachment name/title
   */
  name?: string;

  /**
   * File size in bytes
   */
  size?: number;

  /**
   * MIME type
   */
  mimeType?: string;
}

/**
 * Webhook payload from Beagle
 */
export interface BeagleWebhookPayload {
  /**
   * Event type
   */
  event: 'message.created' | 'message.updated' | 'message.deleted';

  /**
   * Message data
   */
  message: BeagleMessage;

  /**
   * Webhook signature for verification
   */
  signature?: string;
}

/**
 * Response from Beagle API when sending a message
 */
export interface BeagleSendMessageResponse {
  /**
   * Success status
   */
  success: boolean;

  /**
   * Message ID if successful
   */
  messageId?: string;

  /**
   * Error message if failed
   */
  error?: string;
}
