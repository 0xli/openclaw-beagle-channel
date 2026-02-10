/**
 * TypeScript wrapper for the Beagle Carrier native addon
 */

export interface BeagleCarrierConfig {
    dataDir: string;
    bootstrapNodes: string;
}

export interface BeagleCarrierNative {
    initialize(dataDir: string, bootstrapNodes: string): boolean;
    start(): boolean;
    stop(): void;
    isReady(): boolean;
    getAddress(): string;
    getUserId(): string;
    setUserInfo(name: string, description: string): boolean;
    addFriend(address: string, hello: string): boolean;
    sendMessage(friendId: string, message: string): boolean;
    onConnectionChanged(callback: (connected: boolean) => void): void;
    onFriendRequest(callback: (userId: string, name: string, hello: string) => void): void;
    onFriendAdded(callback: (userId: string) => void): void;
    onMessage(callback: (friendId: string, message: string) => void): void;
}

export class BeagleCarrier {
    private native: BeagleCarrierNative | null = null;
    private config: BeagleCarrierConfig | null = null;
    
    constructor() {
        try {
            // Load the native addon
            // @ts-ignore - native module
            const addon = require('../../build/Release/beagle_carrier.node');
            this.native = new addon.BeagleCarrier();
        } catch (error) {
            console.error('Failed to load Beagle Carrier native addon:', error);
            // For development, we can continue without the native addon
        }
    }

    initialize(config: BeagleCarrierConfig): boolean {
        this.config = config;
        if (!this.native) {
            console.warn('Native addon not loaded, using mock implementation');
            return true;
        }
        return this.native.initialize(config.dataDir, config.bootstrapNodes);
    }

    start(): boolean {
        if (!this.native) {
            return true;
        }
        return this.native.start();
    }

    stop(): void {
        if (this.native) {
            this.native.stop();
        }
    }

    isReady(): boolean {
        if (!this.native) {
            return true;
        }
        return this.native.isReady();
    }

    getAddress(): string {
        if (!this.native) {
            return 'mock_address';
        }
        return this.native.getAddress();
    }

    getUserId(): string {
        if (!this.native) {
            return 'mock_user_id';
        }
        return this.native.getUserId();
    }

    setUserInfo(name: string, description: string): boolean {
        if (!this.native) {
            return true;
        }
        return this.native.setUserInfo(name, description);
    }

    addFriend(address: string, hello: string): boolean {
        if (!this.native) {
            console.log(`Mock: Adding friend ${address} with hello: ${hello}`);
            return true;
        }
        return this.native.addFriend(address, hello);
    }

    sendMessage(friendId: string, message: string): boolean {
        if (!this.native) {
            console.log(`Mock: Sending message to ${friendId}: ${message}`);
            return true;
        }
        return this.native.sendMessage(friendId, message);
    }

    onConnectionChanged(callback: (connected: boolean) => void): void {
        if (this.native) {
            this.native.onConnectionChanged(callback);
        } else {
            // Mock connection after a delay
            setTimeout(() => callback(true), 1000);
        }
    }

    onFriendRequest(callback: (userId: string, name: string, hello: string) => void): void {
        if (this.native) {
            this.native.onFriendRequest(callback);
        }
    }

    onFriendAdded(callback: (userId: string) => void): void {
        if (this.native) {
            this.native.onFriendAdded(callback);
        }
    }

    onMessage(callback: (friendId: string, message: string) => void): void {
        if (this.native) {
            this.native.onMessage(callback);
        }
    }
}
