import { BeagleChannelProvider } from 'openclaw-beagle-channel';

/**
 * Basic Example: Send and receive messages via Beagle Chat
 */

async function main() {
  // Create the provider
  const beagleProvider = new BeagleChannelProvider({
    sidecarUrl: 'http://localhost:8765',
    authToken: process.env.BEAGLE_AUTH_TOKEN || '',
    debug: true,
  });

  console.log('Initializing Beagle Chat provider...');

  // Initialize the connection
  await beagleProvider.initialize();

  console.log('Connected to Beagle Chat!');
  console.log('Channel ID:', beagleProvider.getChannelId());
  console.log('Display Name:', beagleProvider.getDisplayName());

  // Set up message handler
  beagleProvider.onIncomingMessage(async (message) => {
    console.log('\n📨 New message received:');
    console.log('  From:', message.peerId);
    console.log('  Text:', message.text);
    console.log('  ID:', message.messageId);
    console.log('  Time:', new Date(message.timestamp).toISOString());

    // Auto-reply to greetings
    if (message.text.toLowerCase().includes('hello')) {
      console.log('  → Sending auto-reply...');
      await beagleProvider.sendText(
        message.peerId,
        'Hello! I am an OpenClaw agent powered by Beagle Chat.'
      );
    }
  });

  // Send a test message
  const testPeerId = process.env.TEST_PEER_ID;
  if (testPeerId) {
    console.log('\n📤 Sending test message to:', testPeerId);
    
    const result = await beagleProvider.sendText(
      testPeerId,
      'Hello from OpenClaw! This is a test message.'
    );

    console.log('  ✅ Message sent successfully');
    console.log('  Message ID:', result.messageId);
    console.log('  Timestamp:', new Date(result.timestamp).toISOString());
  } else {
    console.log('\n⚠️  Set TEST_PEER_ID environment variable to send a test message');
  }

  // Keep the connection alive
  console.log('\n🔌 Connection established. Waiting for messages...');
  console.log('Press Ctrl+C to exit\n');

  // Graceful shutdown on SIGINT/SIGTERM
  process.on('SIGINT', async () => {
    console.log('\n\nShutting down gracefully...');
    await beagleProvider.shutdown();
    console.log('Goodbye!');
    process.exit(0);
  });

  process.on('SIGTERM', async () => {
    console.log('\n\nShutting down gracefully...');
    await beagleProvider.shutdown();
    console.log('Goodbye!');
    process.exit(0);
  });
}

// Run the example
main().catch((error) => {
  console.error('Error:', error);
  process.exit(1);
});
