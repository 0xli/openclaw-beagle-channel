import axios, { AxiosInstance } from 'axios';
import WebSocket from 'ws';
import { BeagleConfig, BeagleMessage, SendMessageRequest, SendMessageResponse, InboundMessageHandler } from './types';

/**
 * Beagle Chat Channel Client
 * 
 * Communicates with the Carrier sidecar daemon to send/receive messages.
 */
export class BeagleChannelClient {
  private config: BeagleConfig;
  private httpClient: AxiosInstance;
  private ws: WebSocket | null = null;
  private messageHandlers: InboundMessageHandler[] = [];
  private reconnectTimer: NodeJS.Timeout | null = null;

  constructor(config: BeagleConfig) {
    this.config = {
      reconnectInterval: 5000,
      debug: false,
      ...config,
    };

    this.httpClient = axios.create({
      baseURL: this.config.sidecarUrl,
      headers: this.config.authToken
        ? { Authorization: `Bearer ${this.config.authToken}` }
        : {},
      timeout: 10000,
    });
  }

  /**
   * Send a text message to a peer
   */
  async sendMessage(request: SendMessageRequest): Promise<SendMessageResponse> {
    try {
      const response = await this.httpClient.post<SendMessageResponse>('/send', request);
      
      if (this.config.debug) {
        console.log('[BeagleChannel] Message sent:', response.data);
      }
      
      return response.data;
    } catch (error) {
      if (this.config.debug) {
        console.error('[BeagleChannel] Send error:', error);
      }
      throw new Error(`Failed to send message: ${error}`);
    }
  }

  /**
   * Register a handler for inbound messages
   */
  onMessage(handler: InboundMessageHandler): void {
    this.messageHandlers.push(handler);
  }

  /**
   * Connect to the sidecar event stream
   */
  async connect(): Promise<void> {
    if (this.ws) {
      return; // Already connected
    }

    const wsUrl = this.config.sidecarUrl.replace(/^http/, 'ws') + '/events';
    
    if (this.config.debug) {
      console.log('[BeagleChannel] Connecting to:', wsUrl);
    }

    this.ws = new WebSocket(wsUrl, {
      headers: this.config.authToken
        ? { Authorization: `Bearer ${this.config.authToken}` }
        : {},
    });

    this.ws.on('open', () => {
      if (this.config.debug) {
        console.log('[BeagleChannel] Connected to sidecar');
      }
      
      // Clear reconnect timer on successful connection
      if (this.reconnectTimer) {
        clearTimeout(this.reconnectTimer);
        this.reconnectTimer = null;
      }
    });

    this.ws.on('message', (data: WebSocket.Data) => {
      try {
        const message: BeagleMessage = JSON.parse(data.toString());
        
        if (this.config.debug) {
          console.log('[BeagleChannel] Received message:', message);
        }
        
        // Call all registered handlers
        this.messageHandlers.forEach(handler => {
          try {
            handler(message);
          } catch (error) {
            console.error('[BeagleChannel] Handler error:', error);
          }
        });
      } catch (error) {
        console.error('[BeagleChannel] Parse error:', error);
      }
    });

    this.ws.on('error', (error: Error) => {
      console.error('[BeagleChannel] WebSocket error:', error);
    });

    this.ws.on('close', () => {
      if (this.config.debug) {
        console.log('[BeagleChannel] Disconnected from sidecar');
      }
      
      this.ws = null;
      
      // Schedule reconnect
      if (!this.reconnectTimer) {
        this.reconnectTimer = setTimeout(() => {
          this.reconnectTimer = null;
          this.connect().catch(err => {
            console.error('[BeagleChannel] Reconnect failed:', err);
          });
        }, this.config.reconnectInterval);
      }
    });
  }

  /**
   * Disconnect from the sidecar
   */
  async disconnect(): Promise<void> {
    if (this.reconnectTimer) {
      clearTimeout(this.reconnectTimer);
      this.reconnectTimer = null;
    }

    if (this.ws) {
      this.ws.close();
      this.ws = null;
    }
  }

  /**
   * Check if connected to sidecar
   */
  isConnected(): boolean {
    return this.ws !== null && this.ws.readyState === WebSocket.OPEN;
  }

  /**
   * Health check - verify sidecar is running
   */
  async healthCheck(): Promise<boolean> {
    try {
      const response = await this.httpClient.get('/health');
      return response.status === 200;
    } catch (error) {
      return false;
    }
  }
}
