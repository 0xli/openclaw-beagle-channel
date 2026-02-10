/**
 * Beagle/Elastos Carrier Channel Plugin for OpenClaw
 * 
 * This plugin enables OpenClaw to communicate over the Elastos Carrier network,
 * allowing Beagle chat clients and other Carrier-based applications to interact
 * with OpenClaw agents.
 */

import { getBeagleRuntime, setBeagleRuntime, BeagleChannelRuntime } from './runtime.js';

// Note: These types would normally be imported from 'openclaw/plugin-sdk'
// For this standalone implementation, we define minimal interfaces

interface OpenClawPluginApi {
    runtime: any;
    registerChannel: (config: { plugin: any }) => void;
}

interface ChannelPlugin {
    id: string;
    meta: {
        id: string;
        label: string;
        selectionLabel: string;
        detailLabel: string;
        docsPath?: string;
        docsLabel?: string;
        blurb?: string;
    };
    capabilities?: {
        chatTypes?: string[];
        reactions?: boolean;
        threads?: boolean;
        media?: boolean;
        nativeCommands?: boolean;
        blockStreaming?: boolean;
    };
    lifecycle: {
        start?: () => Promise<void>;
        stop?: () => Promise<void>;
    };
    send?: (params: any) => Promise<void>;
}

const beaglePlugin: ChannelPlugin = {
    id: 'beagle',
    meta: {
        id: 'beagle',
        label: 'Beagle',
        selectionLabel: 'Beagle (Elastos Carrier)',
        detailLabel: 'Beagle Channel',
        docsPath: '/channels/beagle',
        docsLabel: 'beagle',
        blurb: 'Decentralized peer-to-peer messaging via Elastos Carrier network.',
    },
    capabilities: {
        chatTypes: ['direct'],
        reactions: false,
        threads: false,
        media: false,
        nativeCommands: false,
        blockStreaming: true,
    },
    lifecycle: {
        start: async () => {
            const runtime = getBeagleRuntime();
            
            // Get configuration (in real implementation, this would come from OpenClaw config)
            const config = {
                enabled: true,
                dataDir: process.env.BEAGLE_DATA_DIR || './data/beagle',
                bootstrapNodes: process.env.BEAGLE_BOOTSTRAP_NODES,
                userName: process.env.BEAGLE_USER_NAME || 'OpenClaw Agent',
                userDescription: process.env.BEAGLE_USER_DESC || 'OpenClaw AI Assistant',
            };

            await runtime.initialize(config);
            await runtime.start();

            // Set up message handler
            runtime.onMessage((message) => {
                console.log(`Received message from ${message.from}: ${message.text}`);
                // In real implementation, this would forward to OpenClaw's message processing
            });

            // Set up friend request handler
            runtime.onFriendRequest((userId, name, hello) => {
                console.log(`Friend request from ${name}: ${hello}`);
                // Auto-accept for now (in production, this should follow pairing rules)
            });

            console.log('Beagle channel started successfully');
        },
        stop: async () => {
            const runtime = getBeagleRuntime();
            await runtime.stop();
            console.log('Beagle channel stopped');
        },
    },
    send: async (params: { to: string; message: string }) => {
        const runtime = getBeagleRuntime();
        const success = await runtime.sendMessage(params.to, params.message);
        if (!success) {
            throw new Error('Failed to send message via Beagle');
        }
    },
};

const plugin = {
    id: 'beagle',
    name: 'Beagle Channel',
    description: 'Beagle/Elastos Carrier channel plugin for OpenClaw',
    configSchema: {
        type: 'object',
        additionalProperties: false,
        properties: {},
    },
    register(api: OpenClawPluginApi) {
        // Initialize runtime with OpenClaw's runtime API
        const runtime = new BeagleChannelRuntime();
        setBeagleRuntime(runtime);
        
        // Register the channel
        api.registerChannel({ plugin: beaglePlugin });
        
        console.log('Beagle channel plugin registered');
    },
};

export default plugin;
