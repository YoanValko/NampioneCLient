#include "nemotron_common.h"
#include "jni_helper.h"
#include <windows.h>
#include <jni.h>
#include <string>
#include <thread>
#include <chrono>
#include <iostream>
#include <fstream>
#include <json/json.h>

// Global state
namespace Nemotron {
    JNICache g_jniCache;
    JavaVM* g_javaVM = nullptr;
    JNIEnv* g_jniEnv = nullptr;
    bool g_initialized = false;
    bool g_shutdownRequested = false;
    
    // Module states
    bool g_autoClickerEnabled = false;
    bool g_espEnabled = false;
    bool g_aimbotEnabled = false;
    
    AutoClickerSettings g_autoClickerSettings;
    ESPSettings g_espSettings;
    AimbotSettings g_aimbotSettings;
    
    // Timing
    DWORD g_lastLeftClick = 0;
    DWORD g_lastRightClick = 0;
    std::thread g_mainThread;
    HANDLE g_threadHandle = nullptr;
    
    // IPC
    HANDLE g_pipeHandle = nullptr;
    std::thread g_ipcThread;
}

// Forward declarations
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved);
void InitializeNemotron(JNIEnv* env);
void NemotronMainLoop();
void ShutdownNemotron();
void ProcessIPC();
void HandleCommand(const Nemotron::InjectionPayload& payload, Nemotron::InjectionResponse& response);

// Module implementations
void AutoClickerTick(JNIEnv* env);
void ESPTick(JNIEnv* env);
void AimbotTick(JNIEnv* env);

// JNI exports
extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    Nemotron::g_javaVM = vm;
    JNIEnv* env;
    if (vm->GetEnv(reinterpret_cast<void**>(&Nemotron::g_jniEnv), JNI_VERSION_1_8) != JNI_OK) {
        return JNI_ERR;
    }
    
    Nemotron::JNIHelper::CacheJNIEnvironment(Nemotron::g_jniEnv);
    Nemotron::g_initialized = Nemotron::JNIHelper::InitializeJNICache(Nemotron::g_jniEnv);
    
    if (Nemotron::g_initialized) {
        // Start main thread
        Nemotron::g_mainThread = std::thread(Nemotron::NemotronMainLoop);
        Nemotron::g_threadHandle = Nemotron::g_mainThread.native_handle();
        
        // Start IPC thread
        Nemotron::g_ipcThread = std::thread(Nemotron::ProcessIPC);
    }
    
    return JNI_VERSION_1_8;
}

extern "C" JNIEXPORT void JNICALL JNI_OnUnload(JavaVM* vm, void* reserved) {
    Nemotron::ShutdownNemotron();
}

// DllMain for Windows
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
    switch (reason) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hModule);
            break;
        case DLL_PROCESS_DETACH:
            Nemotron::ShutdownNemotron();
            break;
    }
    return TRUE;
}

// Main initialization
void InitializeNemotron(JNIEnv* env) {
    if (Nemotron::g_initialized) return;
    
    Nemotron::JNIHelper::CacheJNIEnvironment(env);
    Nemotron::g_initialized = Nemotron::JNIHelper::InitializeJNICache(env);
    
    if (Nemotron::g_initialized) {
        // Start main thread
        Nemotron::g_mainThread = std::thread(Nemotron::NemotronMainLoop);
        Nemotron::g_threadHandle = Nemotron::g_mainThread.native_handle();
        
        // Start IPC thread
        Nemotron::g_ipcThread = std::thread(Nemotron::ProcessIPC);
    }
}

// Shutdown
void Nemotron::ShutdownNemotron() {
    Nemotron::g_shutdownRequested = true;
    
    if (Nemotron::g_mainThread.joinable()) {
        Nemotron::g_mainThread.join();
    }
    if (Nemotron::g_ipcThread.joinable()) {
        Nemotron::g_ipcThread.join();
    }
    
    if (Nemotron::g_pipeHandle) {
        CloseHandle(Nemotron::g_pipeHandle);
        Nemotron::g_pipeHandle = nullptr;
    }
    
    Nemotron::g_initialized = false;
}

// IPC handling
void Nemotron::ProcessIPC() {
    while (!Nemotron::g_shutdownRequested) {
        Nemotron::g_pipeHandle = CreateNamedPipeA(
            Nemotron::PIPE_NAME,
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            1,
            Nemotron::SHARED_MEMORY_SIZE,
            Nemotron::SHARED_MEMORY_SIZE,
            0,
            nullptr
        );
        
        if (Nemotron::g_pipeHandle == INVALID_HANDLE_VALUE) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        
        if (ConnectNamedPipe(Nemotron::g_pipeHandle, nullptr) || GetLastError() == ERROR_PIPE_CONNECTED) {
            char buffer[Nemotron::SHARED_MEMORY_SIZE];
            DWORD bytesRead;
            
            while (!Nemotron::g_shutdownRequested) {
                if (ReadFile(Nemotron::g_pipeHandle, buffer, sizeof(buffer), &bytesRead, nullptr) && bytesRead > 0) {
                    InjectionPayload payload;
                    memcpy(&payload, buffer, sizeof(payload));
                    
                    InjectionResponse response = {0};
                    HandleCommand(payload, response);
                    
                    DWORD bytesWritten;
                    WriteFile(Nemotron::g_pipeHandle, &response, sizeof(response), &bytesWritten, nullptr);
                } else {
                    break;
                }
            }
        }
        
        CloseHandle(Nemotron::g_pipeHandle);
        Nemotron::g_pipeHandle = nullptr;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void HandleCommand(const InjectionPayload& payload, InjectionResponse& response) {
    response.success = true;
    strcpy(response.message, "OK");
    
    switch (payload.command) {
        case Command::INIT: {
            Json::Value json;
            json["autoClickerEnabled"] = g_autoClickerEnabled;
            json["espEnabled"] = g_espEnabled;
            json["aimbotEnabled"] = g_aimbotEnabled;
            Json::StreamWriterBuilder writer;
            strcpy(response.json_data, Json::writeString(writer, json).c_str());
            break;
        }
        
        case Command::TOGGLE_MODULE: {
            Json::Value json;
            Json::Reader reader;
            reader.parse(payload.data, json);
            
            int moduleType = json["moduleType"].asInt();
            bool enable = json["enable"].asBool();
            
            switch (static_cast<ModuleType>(moduleType)) {
                case ModuleType::AUTOMATIC_CLICKER:
                    g_autoClickerEnabled = enable;
                    break;
                case ModuleType::ESP:
                    g_espEnabled = enable;
                    break;
                case ModuleType::AIMBOT:
                    g_aimbotEnabled = enable;
                    break;
            }
            
            Json::Value resp;
            resp["success"] = true;
            resp["moduleType"] = moduleType;
            resp["enabled"] = enable;
            Json::StreamWriterBuilder writer;
            strcpy(response.json_data, Json::writeString(writer, resp).c_str());
            break;
        }
        
        case Command::SET_SETTING: {
            Json::Value json;
            Json::Reader reader;
            reader.parse(payload.data, json);
            
            int moduleType = json["moduleType"].asInt();
            
            switch (static_cast<ModuleType>(moduleType)) {
                case ModuleType::AUTOMATIC_CLICKER: {
                    g_autoClickerSettings.cps_min = json["cps_min"].asInt();
                    g_autoClickerSettings.cps_max = json["cps_max"].asInt();
                    g_autoClickerSettings.jitter = json["jitter"].asBool();
                    g_autoClickerSettings.left_click = json["left_click"].asBool();
                    g_autoClickerSettings.right_click = json["right_click"].asBool();
                    g_autoClickerSettings.only_weapon = json["only_weapon"].asBool();
                    g_autoClickerSettings.hit_delay = json["hit_delay"].asInt();
                    break;
                }
                case ModuleType::ESP: {
                    g_espSettings.players = json["players"].asBool();
                    g_espSettings.mobs = json["mobs"].asBool();
                    g_espSettings.animals = json["animals"].asBool();
                    g_espSettings.chests = json["chests"].asBool();
                    g_espSettings.tracers = json["tracers"].asBool();
                    g_espSettings.boxes = json["boxes"].asBool();
                    g_espSettings.health = json["health"].asBool();
                    g_espSettings.max_distance = json["max_distance"].asFloat();
                    break;
                }
                case ModuleType::AIMBOT: {
                    g_aimbotSettings.fov = json["fov"].asFloat();
                    g_aimbotSettings.smooth = json["smooth"].asFloat();
                    g_aimbotSettings.prediction = json["prediction"].asBool();
                    g_aimbotSettings.auto_switch = json["auto_switch"].asBool();
                    g_aimbotSettings.target_mode = json["target_mode"].asInt();
                    g_aimbotSettings.hitbox = json["hitbox"].asInt();
                    g_aimbotSettings.max_distance = json["max_distance"].asFloat();
                    g_aimbotSettings.visible_check = json["visible_check"].asBool();
                    break;
                }
            }
            
            Json::Value resp;
            resp["success"] = true;
            Json::StreamWriterBuilder writer;
            strcpy(response.json_data, Json::writeString(writer, resp).c_str());
            break;
        }
        
        case Command::GET_STATUS: {
            Json::Value json;
            json["autoClickerEnabled"] = g_autoClickerEnabled;
            json["espEnabled"] = g_espEnabled;
            json["aimbotEnabled"] = g_aimbotEnabled;
            json["initialized"] = g_initialized;
            Json::StreamWriterBuilder writer;
            strcpy(response.json_data, Json::writeString(writer, json).c_str());
            break;
        }
        
        case Command::EXECUTE_JAVA: {
            // Execute arbitrary Java code via JNI
            response.success = false;
            strcpy(response.message, "Java execution not implemented in C++ version");
            break;
        }
        
        case Command::SHUTDOWN: {
            g_shutdownRequested = true;
            break;
        }
    }
}

// Module implementations
void AutoClickerTick(JNIEnv* env) {
    if (!g_autoClickerEnabled) return;
    
    JNIHelper::CacheJNIEnvironment(env);
    if (!g_initialized) return;
    
    jobject minecraft = env->GetStaticObjectField(g_jniCache.minecraft_class, g_jniCache.minecraft_thePlayer);
    if (!minecraft) return;
    
    jobject player = env->GetObjectField(minecraft, g_jniCache.minecraft_thePlayer);
    if (!player) return;
    
    // Check if holding weapon (if configured)
    if (g_autoClickerSettings.only_weapon) {
        // Check held item
    }
    
    DWORD now = GetTickCount();
    int cps = g_autoClickerSettings.cps_min + (rand() % (g_autoClickerSettings.cps_max - g_autoClickerSettings.cps_min + 1));
    int delay = 1000 / cps;
    
    if (g_autoClickerSettings.jitter) {
        delay += (rand() % 30) - 15; // ±15ms jitter
    }
    delay += g_autoClickerSettings.hit_delay;
    
    if (g_autoClickerSettings.left_click && (now - g_lastLeftClick >= static_cast<DWORD>(delay))) {
        jobject mc = env->GetStaticObjectField(g_jniCache.minecraft_class, g_jniCache.minecraft_thePlayer);
        env->CallVoidMethod(mc, g_jniCache.minecraft_clickMouse);
        g_lastLeftClick = now;
        
        // Swing arm
        env->CallVoidMethod(player, g_jniCache.entityPlayerSP_swingItem);
    }
    
    if (g_autoClickerSettings.right_click && (now - g_lastRightClick >= static_cast<DWORD>(delay))) {
        jobject mc = env->GetStaticObjectField(g_jniCache.minecraft_class, g_jniCache.minecraft_thePlayer);
        env->CallVoidMethod(mc, g_jniCache.minecraft_rightClickMouse);
        g_lastRightClick = now;
        
        env->CallVoidMethod(player, g_jniCache.entityPlayerSP_swingItem);
    }
}

void ESPTick(JNIEnv* env) {
    // ESP rendering would need to hook into render events
    // This is a placeholder - actual ESP needs render hooks
    if (!g_espEnabled) return;
    JNIHelper::CacheJNIEnvironment(env);
    // Actual ESP implementation would render in RenderWorldLastEvent
}

void AimbotTick(JNIEnv* env) {
    if (!g_aimbotEnabled) return;
    
    JNIHelper::CacheJNIEnvironment(env);
    if (!g_initialized) return;
    
    jobject minecraft = env->GetStaticObjectField(g_jniCache.minecraft_class, g_jniCache.minecraft_thePlayer);
    if (!minecraft) return;
    
    jobject player = env->GetObjectField(minecraft, g_jniCache.minecraft_thePlayer);
    if (!player) return;
    
    jobject world = env->GetObjectField(minecraft, g_jniCache.minecraft_theWorld);
    if (!world) return;
    
    // Get player position
    jdouble playerX = env->GetDoubleField(player, g_jniCache.entityPlayerSP_posX);
    jdouble playerY = env->GetDoubleField(player, g_jniCache.entityPlayerSP_posY);
    jdouble playerZ = env->GetDoubleField(player, g_jniCache.entityPlayerSP_posZ);
    
    // Get player rotation
    jfloat playerYaw = env->GetFloatField(player, GetFieldID(env, g_jniCache.entityPlayerSP_class, "rotationYaw", "F"));
    jfloat playerPitch = env->GetFloatField(player, GetFieldID(env, g_jniCache.entityPlayerSP_class, "rotationPitch", "F"));
    
    // Get loaded entities
    jobject entityList = env->GetObjectField(world, g_jniCache.world_loadedEntityList);
    if (!entityList) return;
    
    jsize listSize = env->GetArrayLength(reinterpret_cast<jarray>(entityList));
    if (listSize <= 0) return;
    
    Vec3 playerPos(static_cast<float>(playerX), static_cast<float>(playerY + 1.62), static_cast<float>(playerZ));
    Rotation currentRot(playerYaw, playerPitch);
    
    // Find best target
    float bestFOV = g_aimbotSettings.fov;
    jobject bestTarget = nullptr;
    Vec3 bestTargetPos;
    
    for (int i = 0; i < listSize; i++) {
        jobject entity = env->GetObjectArrayElement(reinterpret_cast<jobjectArray>(entityList), i);
        if (!entity) continue;
        
        // Skip dead entities
        jboolean isDead = env->CallBooleanMethod(entity, g_jniCache.entityLivingBase_isDead);
        if (isDead) continue;
        
        // Skip self
        if (env->IsSameObject(entity, player)) continue;
        
        // Get entity position
        jdouble ex = env->GetDoubleField(entity, g_jniCache.entity_posX);
        jdouble ey = env->GetDoubleField(entity, g_jniCache.entity_posY);
        jdouble ez = env->GetDoubleField(entity, g_jniCache.entity_posZ);
        
        Vec3 entityPos(static_cast<float>(ex), static_cast<float>(ey), static_cast<float>(ez));
        
        // Distance check
        float distance = playerPos.DistanceTo(entityPos);
        if (distance > g_aimbotSettings.max_distance) continue;
        
        // Calculate angle to target
        Vec3 targetPos = entityPos;
        targetPos.y += 1.6f; // Aim at head level
        Rotation targetAngle = CalculateAngle(playerPos, targetAngle);
        
        // Calculate FOV difference
        Rotation diff(targetAngle.yaw - currentRot.yaw, targetAngle.pitch - currentRot.pitch);
        NormalizeAngles(diff);
        float fov = sqrtf(diff.yaw * diff.yaw + diff.pitch * diff.pitch);
        
        if (fov < bestFOV) {
            bestFOV = fov;
            bestTarget = entity;
            bestTargetPos = targetPos;
        }
    }
    
    // Aim at target
    if (bestTarget) {
        Rotation targetAngle = CalculateAngle(playerPos, bestTargetPos);
        NormalizeAngles(targetAngle);
        
        // Smooth aiming
        Rotation smoothed = SmoothRotation(currentRot, targetAngle, g_aimbotSettings.smooth);
        
        // Apply rotation (humanized)
        if (g_aimbotSettings.smooth > 1.0f) {
            // Add micro-jitter
            smoothed.yaw += (RandomFloat(-1.0f, 1.0f) * 0.1f);
            smoothed.pitch += (RandomFloat(-1.0f, 1.0f) * 0.1f);
        }
        
        // Apply to player
        env->SetFloatField(player, GetFieldID(env, g_jniCache.entityPlayerSP_class, "rotationYaw", "F"), smoothed.yaw);
        env->SetFloatField(player, GetFieldID(env, g_jniCache.entityPlayerSP_class, "rotationPitch", "F"), smoothed.pitch);
    }
}

// Main loop
void NemotronMainLoop() {
    // Wait for JVM to be ready
    while (!g_initialized && !g_shutdownRequested) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    while (!g_shutdownRequested) {
        if (g_initialized && g_javaVM) {
            JNIEnv* env;
            jint result = g_javaVM->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_8);
            if (result == JNI_OK) {
                JNIHelper::CacheJNIEnvironment(env);
                
                // Run modules
                if (g_autoClickerEnabled) AutoClickerTick(env);
                if (g_espEnabled) ESPTick(env);
                if (g_aimbotEnabled) AimbotTick(env);
            } else if (result == JNI_EDETACHED) {
                // Attach current thread
                g_javaVM->AttachCurrentThread(reinterpret_cast<void**>(&env), nullptr);
                JNIHelper::CacheJNIEnvironment(env);
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1)); // ~1000 FPS cap
    }
}

// Export functions for external use
extern "C" {
    __declspec(dllexport) bool IsNemotronLoaded() {
        return g_initialized;
    }
    
    __declspec(dllexport) void ToggleAutoClicker(bool enable) {
        g_autoClickerEnabled = enable;
    }
    
    __declspec(dllexport) void ToggleESP(bool enable) {
        g_espEnabled = enable;
    }
    
    __declspec(dllexport) void ToggleAimbot(bool enable) {
        g_aimbotEnabled = enable;
    }
    
    __declspec(dllexport) void SetAutoClickerSettings(const AutoClickerSettings* settings) {
        if (settings) g_autoClickerSettings = *settings;
    }
    
    __declspec(dllexport) void SetESPSettings(const ESPSettings* settings) {
        if (settings) g_espSettings = *settings;
    }
    
    __declspec(dllexport) void SetAimbotSettings(const AimbotSettings* settings) {
        if (settings) g_aimbotSettings = *settings;
    }
}