/**
 * Example usage of the OpenClaw Beagle Channel Plugin
 * 
 * This example demonstrates how to:
 * 1. Initialize the Beagle channel
 * 2. Set up a message handler
 * 3. Send messages to users
 * 4. Handle webhooks from Beagle Chat
 */

import { BeagleChannel } from './index.js';
import type { BeagleChannelConfig } from './types.js';

// Configuration from environment variables
const config: BeagleChannelConfig = {
  apiUrl: process.env.BEAGLE_API_URL || 'https://api.beagle.example.com/v1',
  authToken: process.env.BEAGLE_AUTH_TOKEN || 'your-api-token',
  webhookSecret: process.env.BEAGLE_WEBHOOK_SECRET,
  debug: process.env.BEAGLE_DEBUG === 'true',
};

// Create and initialize the channel
const channel = new BeagleChannel();

async function main() {
  console.log('Initializing Beagle channel...');
  
  // Initialize the channel
  await channel.initialize(config);
  console.log('Channel initialized successfully!');

  // Set up message handler
  channel.onMessage(async (message) => {
    console.log(`\n📨 New message from ${message.userName} (${message.userId}):`);
    console.log(`   "${message.text}"`);
    console.log(`   Message ID: ${message.messageId}`);
    console.log(`   Timestamp: ${new Date(message.timestamp).toISOString()}`);

    // Example: Echo the message back
    const response = `Hello ${message.userName}! You said: "${message.text}"`;
    
    try {
      await channel.sendMessage(message.userId, response);
      console.log(`✅ Sent response to ${message.userName}`);
    } catch (error) {
      console.error(`❌ Failed to send response:`, error);
    }
  });

  console.log('Message handler registered. Waiting for messages...');
  console.log('Press Ctrl+C to exit\n');

  // Example: Send a test message
  // Uncomment to test sending a message
  // await channel.sendMessage('test-user-id', 'Hello from OpenClaw Beagle channel!');

  // Keep the process running
  process.on('SIGINT', async () => {
    console.log('\n\nShutting down...');
    await channel.disconnect();
    process.exit(0);
  });
}

// Run the example
main().catch((error) => {
  console.error('Error running example:', error);
  process.exit(1);
});

/**
 * Example webhook server using Express
 * 
 * To use this, install express:
 *   npm install express @types/express
 * 
 * Then uncomment the code below and run with:
 *   node --loader tsx example.ts
 */

/*
import express from 'express';

const app = express();
app.use(express.json());

// Webhook endpoint
app.post('/webhook/beagle', async (req, res) => {
  console.log('Received webhook:', req.body);
  
  try {
    await channel.handleWebhook(req.body);
    res.status(200).json({ success: true });
  } catch (error) {
    console.error('Webhook error:', error);
    res.status(500).json({ 
      success: false, 
      error: error instanceof Error ? error.message : 'Unknown error' 
    });
  }
});

// Health check endpoint
app.get('/health', (req, res) => {
  res.json({ status: 'ok', channel: 'beagle' });
});

// Start the webhook server
const port = process.env.BEAGLE_WEBHOOK_PORT || 3000;
app.listen(port, () => {
  console.log(`Webhook server listening on port ${port}`);
  console.log(`Webhook URL: http://localhost:${port}/webhook/beagle`);
});
*/
