#!/usr/bin/env node

const { BeagleChannelPlugin, MediaType } = require('../dist');

async function test() {
  console.log('=== Testing Beagle Channel Plugin ===\n');

  // Create plugin instance
  const plugin = new BeagleChannelPlugin('http://localhost:8080');
  console.log('✓ Plugin created');
  console.log('  Name:', plugin.name);
  console.log('  Capabilities:', JSON.stringify(plugin.capabilities, null, 2));

  // Initialize
  await plugin.initialize();
  console.log('\n✓ Plugin initialized');

  // Set up message handler
  plugin.onMessage((message) => {
    console.log('\n✓ Received inbound message:');
    console.log('  From:', message.peer);
    console.log('  Text:', message.text);
    console.log('  Media Type:', message.mediaType);
    console.log('  Media URL:', message.mediaUrl);
    console.log('  Timestamp:', new Date(message.timestamp).toISOString());
  });

  // Test 1: Send text message
  console.log('\nTest 1: Sending text message...');
  await plugin.sendText({
    channelId: 'beagle',
    peer: 'test-user',
    text: 'Hello from OpenClaw!'
  });
  console.log('✓ Text message sent');

  // Test 2: Send media with path
  console.log('\nTest 2: Sending media with path...');
  await plugin.sendMedia({
    channelId: 'beagle',
    peer: 'test-user',
    mediaPath: '/tmp/test-image.jpg',
    mediaType: MediaType.Image,
    text: 'Check out this image!'
  });
  console.log('✓ Media message sent (path)');

  // Test 3: Send media with URL
  console.log('\nTest 3: Sending media with URL...');
  await plugin.sendMedia({
    channelId: 'beagle',
    peer: 'test-user',
    mediaUrl: 'https://example.com/document.pdf',
    mediaType: MediaType.Document,
    filename: 'document.pdf',
    mimeType: 'application/pdf'
  });
  console.log('✓ Media message sent (URL)');

  console.log('\n=== All tests passed! ===');
  console.log('Plugin is ready for integration with OpenClaw');

  // Cleanup
  await plugin.cleanup();
  console.log('\n✓ Cleanup complete');
  
  process.exit(0);
}

test().catch((error) => {
  console.error('✗ Test failed:', error.message);
  process.exit(1);
});
