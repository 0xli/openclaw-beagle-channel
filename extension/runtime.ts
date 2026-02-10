/**
 * Beagle/Elastos Carrier Channel for OpenClaw
 * This module provides integration between OpenClaw and Elastos Carrier network
 */

import { BeagleCarrier, type BeagleCarrierConfig } from '../src/beagle-carrier.js';

export interface BeagleChannelConfig {
    enabled?: boolean;
    dataDir?: string;
    bootstrapNodes?: string;
    userName?: string;
    userDescription?: string;
}

export interface BeagleMessage {
    from: string;
    to: string;
    text: string;
    timestamp: number;
}

/**
 * Runtime management for Beagle Carrier channel
 */
export class BeagleChannelRuntime {
    private carrier: BeagleCarrier | null = null;
    private config: BeagleChannelConfig | null = null;
    private messageHandlers: Array<(message: BeagleMessage) => void> = [];
    private friendRequestHandlers: Array<(userId: string, name: string, hello: string) => void> = [];
    private connectionHandlers: Array<(connected: boolean) => void> = [];

    async initialize(config: BeagleChannelConfig): Promise<void> {
        this.config = config;
        this.carrier = new BeagleCarrier();

        const carrierConfig: BeagleCarrierConfig = {
            dataDir: config.dataDir || './data/beagle',
            bootstrapNodes: config.bootstrapNodes || this.getDefaultBootstrapNodes(),
        };

        const initialized = this.carrier.initialize(carrierConfig);
        if (!initialized) {
            throw new Error('Failed to initialize Beagle Carrier');
        }

        // Set up event handlers
        this.carrier.onConnectionChanged((connected) => {
            console.log(`Beagle Carrier connection: ${connected ? 'connected' : 'disconnected'}`);
            this.connectionHandlers.forEach(handler => handler(connected));
        });

        this.carrier.onMessage((friendId, message) => {
            const beagleMessage: BeagleMessage = {
                from: friendId,
                to: this.carrier?.getUserId() || '',
                text: message,
                timestamp: Date.now(),
            };
            this.messageHandlers.forEach(handler => handler(beagleMessage));
        });

        this.carrier.onFriendRequest((userId, name, hello) => {
            console.log(`Friend request from ${name} (${userId}): ${hello}`);
            this.friendRequestHandlers.forEach(handler => handler(userId, name, hello));
        });

        this.carrier.onFriendAdded((userId) => {
            console.log(`Friend added: ${userId}`);
        });

        // Set user info if provided
        if (config.userName || config.userDescription) {
            this.carrier.setUserInfo(
                config.userName || 'OpenClaw Agent',
                config.userDescription || 'OpenClaw AI Assistant via Beagle'
            );
        }
    }

    async start(): Promise<void> {
        if (!this.carrier) {
            throw new Error('Carrier not initialized');
        }

        const started = this.carrier.start();
        if (!started) {
            throw new Error('Failed to start Beagle Carrier');
        }

        console.log('Beagle Carrier started');
        console.log('Carrier Address:', this.carrier.getAddress());
        console.log('User ID:', this.carrier.getUserId());
    }

    async stop(): Promise<void> {
        if (this.carrier) {
            this.carrier.stop();
        }
    }

    isReady(): boolean {
        return this.carrier?.isReady() || false;
    }

    getAddress(): string | null {
        return this.carrier?.getAddress() || null;
    }

    getUserId(): string | null {
        return this.carrier?.getUserId() || null;
    }

    async sendMessage(friendId: string, message: string): Promise<boolean> {
        if (!this.carrier) {
            throw new Error('Carrier not initialized');
        }
        return this.carrier.sendMessage(friendId, message);
    }

    async addFriend(address: string, hello?: string): Promise<boolean> {
        if (!this.carrier) {
            throw new Error('Carrier not initialized');
        }
        return this.carrier.addFriend(address, hello || 'Hello from OpenClaw');
    }

    onMessage(handler: (message: BeagleMessage) => void): void {
        this.messageHandlers.push(handler);
    }

    onFriendRequest(handler: (userId: string, name: string, hello: string) => void): void {
        this.friendRequestHandlers.push(handler);
    }

    onConnectionChanged(handler: (connected: boolean) => void): void {
        this.connectionHandlers.push(handler);
    }

    private getDefaultBootstrapNodes(): string {
        // Default Elastos Carrier bootstrap nodes
        return JSON.stringify([
            {
                "ipv4": "13.58.208.50",
                "port": 33445,
                "publicKey": "89vny8MrKdDKs7Uta9RzoD4AwPbNbMIL8+EgzX5MeZg="
            },
            {
                "ipv4": "18.216.102.47",
                "port": 33445,
                "publicKey": "G5z8MqiNDCYYDt5fQrKaD4AwPbNbMIL8+EgzX5MeZg="
            }
        ]);
    }
}

// Singleton instance
let runtime: BeagleChannelRuntime | null = null;

export function getBeagleRuntime(): BeagleChannelRuntime {
    if (!runtime) {
        runtime = new BeagleChannelRuntime();
    }
    return runtime;
}

export function setBeagleRuntime(rt: BeagleChannelRuntime): void {
    runtime = rt;
}
