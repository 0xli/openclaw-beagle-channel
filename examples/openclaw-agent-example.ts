import { BeagleChannelProvider } from 'openclaw-beagle-channel';

/**
 * OpenClaw Agent Example
 * 
 * This example shows how to integrate Beagle Chat with an OpenClaw agent
 * that can understand and respond to user queries.
 */

interface AgentContext {
  conversationHistory: Map<string, Array<{ role: string; content: string }>>;
}

class BeagleOpenClawAgent {
  private provider: BeagleChannelProvider;
  private context: AgentContext;

  constructor(sidecarUrl: string, authToken?: string) {
    this.provider = new BeagleChannelProvider({
      sidecarUrl,
      authToken,
      debug: true,
    });

    this.context = {
      conversationHistory: new Map(),
    };
  }

  async initialize() {
    console.log('🚀 Initializing Beagle OpenClaw Agent...');
    
    await this.provider.initialize();
    
    console.log('✅ Agent connected to Beagle Chat');
    console.log(`   Channel: ${this.provider.getDisplayName()}`);

    // Set up message handler
    this.provider.onIncomingMessage(async (message) => {
      await this.handleMessage(message.peerId, message.text);
    });
  }

  async handleMessage(peerId: string, text: string) {
    console.log(`\n📨 Message from ${peerId}: ${text}`);

    // Get or create conversation history
    if (!this.context.conversationHistory.has(peerId)) {
      this.context.conversationHistory.set(peerId, []);
    }
    const history = this.context.conversationHistory.get(peerId)!;

    // Add user message to history
    history.push({ role: 'user', content: text });

    // Process the message and generate response
    const response = await this.processMessage(peerId, text, history);

    // Add assistant response to history
    history.push({ role: 'assistant', content: response });

    // Keep only last 10 messages
    if (history.length > 10) {
      history.splice(0, history.length - 10);
    }

    // Send response
    console.log(`📤 Responding: ${response}`);
    await this.provider.sendText(peerId, response);
  }

  async processMessage(
    peerId: string,
    text: string,
    history: Array<{ role: string; content: string }>
  ): Promise<string> {
    // This is a simple rule-based agent
    // In production, you would integrate with OpenClaw's AI capabilities
    
    const lowerText = text.toLowerCase();

    // Handle greetings
    if (lowerText.match(/^(hi|hello|hey)/)) {
      return "Hello! I'm an OpenClaw agent. How can I assist you today?";
    }

    // Handle help requests
    if (lowerText.includes('help')) {
      return "I can help you with various tasks. Try asking me:\n" +
             "• 'What can you do?'\n" +
             "• 'Tell me about Beagle Chat'\n" +
             "• 'What's the weather?' (if configured)";
    }

    // Handle capabilities query
    if (lowerText.includes('what can you')) {
      return "I'm an AI agent powered by OpenClaw, communicating via Beagle Chat (Elastos Carrier). " +
             "I can chat with you, remember our conversation, and help with various tasks.";
    }

    // Handle Beagle Chat info
    if (lowerText.includes('beagle')) {
      return "Beagle Chat uses the Elastos Carrier protocol for secure, decentralized messaging. " +
             "This integration allows OpenClaw agents to communicate via this network.";
    }

    // Handle goodbye
    if (lowerText.match(/^(bye|goodbye|see you)/)) {
      // Clear conversation history on goodbye
      this.context.conversationHistory.delete(peerId);
      return "Goodbye! Feel free to message me anytime.";
    }

    // Default response for unknown queries
    // In production, this would call OpenClaw's AI/LLM
    return "I received your message: \"" + text + "\"\n" +
           "This is a demo agent. In production, I would use OpenClaw's AI to understand and respond to your query.";
  }

  async shutdown() {
    console.log('\n🛑 Shutting down agent...');
    await this.provider.shutdown();
    console.log('✅ Agent stopped');
  }

  isConnected(): boolean {
    return this.provider.isConnected();
  }
}

// Main execution
async function main() {
  const agent = new BeagleOpenClawAgent(
    process.env.SIDECAR_URL || 'http://localhost:8765',
    process.env.BEAGLE_AUTH_TOKEN
  );

  await agent.initialize();

  console.log('\n🎯 Agent is ready and listening for messages...');
  console.log('Press Ctrl+C to stop\n');

  // Monitor connection status
  setInterval(() => {
    const status = agent.isConnected() ? '🟢 Connected' : '🔴 Disconnected';
    process.stdout.write(`\r${status}  `);
  }, 5000);

  // Graceful shutdown
  const shutdown = async () => {
    await agent.shutdown();
    process.exit(0);
  };

  process.on('SIGINT', shutdown);
  process.on('SIGTERM', shutdown);
}

main().catch((error) => {
  console.error('Fatal error:', error);
  process.exit(1);
});
