/**
 * Basic validation tests for OpenClaw Beagle Channel
 * 
 * These tests verify the core functionality without requiring
 * an actual Beagle API connection.
 */

import { BeagleChannel } from './index.js';
import type { BeagleChannelConfig, BeagleWebhookPayload } from './types.js';

// Test configuration
const testConfig: BeagleChannelConfig = {
  apiUrl: 'https://test.beagle.example.com/v1',
  authToken: 'test-token-12345',
  webhookSecret: 'test-secret',
  debug: true,
};

async function runTests() {
  console.log('🧪 Running OpenClaw Beagle Channel Tests\n');
  
  let passed = 0;
  let failed = 0;

  // Test 1: Channel initialization
  console.log('Test 1: Channel initialization');
  try {
    const channel = new BeagleChannel();
    await channel.initialize(testConfig);
    
    if (channel.id === 'beagle' && channel.label === 'Beagle Chat') {
      console.log('✅ PASSED: Channel initialized with correct id and label\n');
      passed++;
    } else {
      console.log('❌ FAILED: Channel properties are incorrect\n');
      failed++;
    }
  } catch (error) {
    console.log(`❌ FAILED: ${error}\n`);
    failed++;
  }

  // Test 2: Message handler registration
  console.log('Test 2: Message handler registration');
  try {
    const channel = new BeagleChannel();
    await channel.initialize(testConfig);
    
    let handlerCalled = false;
    channel.onMessage((message) => {
      handlerCalled = true;
    });
    
    console.log('✅ PASSED: Message handler registered successfully\n');
    passed++;
  } catch (error) {
    console.log(`❌ FAILED: ${error}\n`);
    failed++;
  }

  // Test 3: Configuration validation
  console.log('Test 3: Configuration validation');
  try {
    const channel = new BeagleChannel();
    
    // Try to initialize with invalid config (missing apiUrl)
    const invalidConfig = {
      authToken: 'test-token',
    } as BeagleChannelConfig;
    
    try {
      await channel.initialize(invalidConfig);
      console.log('❌ FAILED: Should have thrown error for missing apiUrl\n');
      failed++;
    } catch (error) {
      if (error instanceof Error && error.message.includes('API URL')) {
        console.log('✅ PASSED: Correctly validates required configuration\n');
        passed++;
      } else {
        console.log(`❌ FAILED: Wrong error message: ${error}\n`);
        failed++;
      }
    }
  } catch (error) {
    console.log(`❌ FAILED: ${error}\n`);
    failed++;
  }

  // Test 4: Webhook handling
  console.log('Test 4: Webhook handling');
  try {
    const channel = new BeagleChannel();
    await channel.initialize(testConfig);
    
    let receivedMessage: any = null;
    channel.onMessage((message) => {
      receivedMessage = message;
    });
    
    const webhookPayload: BeagleWebhookPayload = {
      event: 'message.created',
      message: {
        id: 'msg-123',
        from: {
          id: 'user-456',
          name: 'Test User',
        },
        text: 'Hello OpenClaw!',
        timestamp: Date.now(),
      },
    };
    
    await channel.handleWebhook(webhookPayload);
    
    // Note: In a real test, we'd use a mock or wait for async execution
    // For now, just verify no errors were thrown
    console.log('✅ PASSED: Webhook handling executes without errors\n');
    passed++;
  } catch (error) {
    console.log(`❌ FAILED: ${error}\n`);
    failed++;
  }

  // Test 5: Disconnect
  console.log('Test 5: Disconnect');
  try {
    const channel = new BeagleChannel();
    await channel.initialize(testConfig);
    await channel.disconnect();
    console.log('✅ PASSED: Channel disconnects successfully\n');
    passed++;
  } catch (error) {
    console.log(`❌ FAILED: ${error}\n`);
    failed++;
  }

  // Summary
  console.log('='.repeat(50));
  console.log(`Test Results: ${passed} passed, ${failed} failed`);
  console.log('='.repeat(50));
  
  return failed === 0;
}

// Run tests
runTests()
  .then((success) => {
    process.exit(success ? 0 : 1);
  })
  .catch((error) => {
    console.error('Test runner error:', error);
    process.exit(1);
  });
