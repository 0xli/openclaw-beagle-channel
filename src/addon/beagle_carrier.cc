#include <napi.h>
#include "carrier_wrapper.h"
#include <memory>

class BeagleCarrierAddon : public Napi::ObjectWrap<BeagleCarrierAddon> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    BeagleCarrierAddon(const Napi::CallbackInfo& info);

private:
    std::unique_ptr<CarrierWrapper> carrier_;
    Napi::ThreadSafeFunction tsfnConnectionChanged_;
    Napi::ThreadSafeFunction tsfnFriendRequest_;
    Napi::ThreadSafeFunction tsfnFriendAdded_;
    Napi::ThreadSafeFunction tsfnMessage_;

    // Methods
    Napi::Value Initialize(const Napi::CallbackInfo& info);
    Napi::Value Start(const Napi::CallbackInfo& info);
    void Stop(const Napi::CallbackInfo& info);
    Napi::Value IsReady(const Napi::CallbackInfo& info);
    Napi::Value GetAddress(const Napi::CallbackInfo& info);
    Napi::Value GetUserId(const Napi::CallbackInfo& info);
    Napi::Value SetUserInfo(const Napi::CallbackInfo& info);
    Napi::Value AddFriend(const Napi::CallbackInfo& info);
    Napi::Value SendMessage(const Napi::CallbackInfo& info);
    
    // Callback setters
    void SetOnConnectionChanged(const Napi::CallbackInfo& info);
    void SetOnFriendRequest(const Napi::CallbackInfo& info);
    void SetOnFriendAdded(const Napi::CallbackInfo& info);
    void SetOnMessage(const Napi::CallbackInfo& info);
};

BeagleCarrierAddon::BeagleCarrierAddon(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<BeagleCarrierAddon>(info) {
    carrier_ = std::make_unique<CarrierWrapper>();
}

Napi::Object BeagleCarrierAddon::Init(Napi::Env env, Napi::Object exports) {
    Napi::Function func = DefineClass(env, "BeagleCarrier", {
        InstanceMethod("initialize", &BeagleCarrierAddon::Initialize),
        InstanceMethod("start", &BeagleCarrierAddon::Start),
        InstanceMethod("stop", &BeagleCarrierAddon::Stop),
        InstanceMethod("isReady", &BeagleCarrierAddon::IsReady),
        InstanceMethod("getAddress", &BeagleCarrierAddon::GetAddress),
        InstanceMethod("getUserId", &BeagleCarrierAddon::GetUserId),
        InstanceMethod("setUserInfo", &BeagleCarrierAddon::SetUserInfo),
        InstanceMethod("addFriend", &BeagleCarrierAddon::AddFriend),
        InstanceMethod("sendMessage", &BeagleCarrierAddon::SendMessage),
        InstanceMethod("onConnectionChanged", &BeagleCarrierAddon::SetOnConnectionChanged),
        InstanceMethod("onFriendRequest", &BeagleCarrierAddon::SetOnFriendRequest),
        InstanceMethod("onFriendAdded", &BeagleCarrierAddon::SetOnFriendAdded),
        InstanceMethod("onMessage", &BeagleCarrierAddon::SetOnMessage),
    });

    Napi::FunctionReference* constructor = new Napi::FunctionReference();
    *constructor = Napi::Persistent(func);
    env.SetInstanceData(constructor);

    exports.Set("BeagleCarrier", func);
    return exports;
}

Napi::Value BeagleCarrierAddon::Initialize(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsString()) {
        Napi::TypeError::New(env, "String arguments expected (dataDir, bootstrapNodes)")
            .ThrowAsJavaScriptException();
        return env.Null();
    }

    std::string dataDir = info[0].As<Napi::String>().Utf8Value();
    std::string bootstrapNodes = info[1].As<Napi::String>().Utf8Value();

    bool result = carrier_->Initialize(dataDir, bootstrapNodes);
    return Napi::Boolean::New(env, result);
}

Napi::Value BeagleCarrierAddon::Start(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    bool result = carrier_->Start();
    return Napi::Boolean::New(env, result);
}

void BeagleCarrierAddon::Stop(const Napi::CallbackInfo& info) {
    carrier_->Stop();
}

Napi::Value BeagleCarrierAddon::IsReady(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    return Napi::Boolean::New(env, carrier_->IsReady());
}

Napi::Value BeagleCarrierAddon::GetAddress(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    return Napi::String::New(env, carrier_->GetAddress());
}

Napi::Value BeagleCarrierAddon::GetUserId(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    return Napi::String::New(env, carrier_->GetUserId());
}

Napi::Value BeagleCarrierAddon::SetUserInfo(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsString()) {
        Napi::TypeError::New(env, "String arguments expected (name, description)")
            .ThrowAsJavaScriptException();
        return env.Null();
    }

    std::string name = info[0].As<Napi::String>().Utf8Value();
    std::string description = info[1].As<Napi::String>().Utf8Value();

    bool result = carrier_->SetUserInfo(name, description);
    return Napi::Boolean::New(env, result);
}

Napi::Value BeagleCarrierAddon::AddFriend(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsString()) {
        Napi::TypeError::New(env, "String arguments expected (address, hello)")
            .ThrowAsJavaScriptException();
        return env.Null();
    }

    std::string address = info[0].As<Napi::String>().Utf8Value();
    std::string hello = info[1].As<Napi::String>().Utf8Value();

    bool result = carrier_->AddFriend(address, hello);
    return Napi::Boolean::New(env, result);
}

Napi::Value BeagleCarrierAddon::SendMessage(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsString()) {
        Napi::TypeError::New(env, "String arguments expected (friendId, message)")
            .ThrowAsJavaScriptException();
        return env.Null();
    }

    std::string friendId = info[0].As<Napi::String>().Utf8Value();
    std::string message = info[1].As<Napi::String>().Utf8Value();

    bool result = carrier_->SendMessage(friendId, message);
    return Napi::Boolean::New(env, result);
}

void BeagleCarrierAddon::SetOnConnectionChanged(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsFunction()) {
        Napi::TypeError::New(env, "Function argument expected")
            .ThrowAsJavaScriptException();
        return;
    }

    tsfnConnectionChanged_ = Napi::ThreadSafeFunction::New(
        env,
        info[0].As<Napi::Function>(),
        "ConnectionChangedCallback",
        0,
        1
    );

    carrier_->SetOnConnectionChanged([this](bool connected) {
        tsfnConnectionChanged_.BlockingCall([connected](Napi::Env env, Napi::Function jsCallback) {
            jsCallback.Call({Napi::Boolean::New(env, connected)});
        });
    });
}

void BeagleCarrierAddon::SetOnFriendRequest(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsFunction()) {
        Napi::TypeError::New(env, "Function argument expected")
            .ThrowAsJavaScriptException();
        return;
    }

    tsfnFriendRequest_ = Napi::ThreadSafeFunction::New(
        env,
        info[0].As<Napi::Function>(),
        "FriendRequestCallback",
        0,
        1
    );

    carrier_->SetOnFriendRequest([this](std::string userId, std::string name, std::string hello) {
        tsfnFriendRequest_.BlockingCall([userId, name, hello](Napi::Env env, Napi::Function jsCallback) {
            jsCallback.Call({
                Napi::String::New(env, userId),
                Napi::String::New(env, name),
                Napi::String::New(env, hello)
            });
        });
    });
}

void BeagleCarrierAddon::SetOnFriendAdded(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsFunction()) {
        Napi::TypeError::New(env, "Function argument expected")
            .ThrowAsJavaScriptException();
        return;
    }

    tsfnFriendAdded_ = Napi::ThreadSafeFunction::New(
        env,
        info[0].As<Napi::Function>(),
        "FriendAddedCallback",
        0,
        1
    );

    carrier_->SetOnFriendAdded([this](std::string userId) {
        tsfnFriendAdded_.BlockingCall([userId](Napi::Env env, Napi::Function jsCallback) {
            jsCallback.Call({Napi::String::New(env, userId)});
        });
    });
}

void BeagleCarrierAddon::SetOnMessage(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsFunction()) {
        Napi::TypeError::New(env, "Function argument expected")
            .ThrowAsJavaScriptException();
        return;
    }

    tsfnMessage_ = Napi::ThreadSafeFunction::New(
        env,
        info[0].As<Napi::Function>(),
        "MessageCallback",
        0,
        1
    );

    carrier_->SetOnMessage([this](std::string friendId, std::string message) {
        tsfnMessage_.BlockingCall([friendId, message](Napi::Env env, Napi::Function jsCallback) {
            jsCallback.Call({
                Napi::String::New(env, friendId),
                Napi::String::New(env, message)
            });
        });
    });
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    return BeagleCarrierAddon::Init(env, exports);
}

NODE_API_MODULE(beagle_carrier, Init)
