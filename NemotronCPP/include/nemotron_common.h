#pragma once

#ifndef NEMOTRON_COMMON_H
#define NEMOTRON_COMMON_H

#include <string>
#include <vector>
#include <cstdint>

// Shared structures between injector and DLL
namespace Nemotron {

    // Module types
    enum class ModuleType : uint32_t {
        NONE = 0,
        AUTOMATIC_CLICKER = 1,
        ESP = 2,
        AIMBOT = 3
    };

    // Injection commands
    enum class Command : uint32_t {
        INIT = 0,
        TOGGLE_MODULE = 1,
        SET_SETTING = 2,
        GET_STATUS = 3,
        EXECUTE_JAVA = 4,
        SHUTDOWN = 5
    };

    // Module states
    struct ModuleState {
        ModuleType type;
        bool enabled;
        char name[64];
        char settings_json[1024];
    };

    // Injection payload
    struct InjectionPayload {
        Command command;
        ModuleType module_type;
        char data[2048]; // JSON data
    };

    // Response from DLL
    struct InjectionResponse {
        bool success;
        char message[512];
        char json_data[4096];
    };

    // IPC communication (named pipe)
    constexpr const char* PIPE_NAME = "\\\\.\\pipe\\NemotronIPC";
    constexpr const char* SHARED_MEMORY_NAME = "NemotronSharedMem";
    constexpr size_t SHARED_MEMORY_SIZE = 8192;

    // Module settings structures
    struct AutoClickerSettings {
        int cps_min = 8;
        int cps_max = 12;
        bool jitter = true;
        bool left_click = true;
        bool right_click = false;
        bool only_weapon = false;
        int hit_delay = 0;
    };

    struct ESPSettings {
        bool players = true;
        bool mobs = false;
        bool animals = false;
        bool chests = false;
        bool tracers = true;
        bool boxes = true;
        bool health = true;
        float max_distance = 100.0f;
    };

    struct AimbotSettings {
        float fov = 90.0f;
        float smooth = 5.0f;
        bool prediction = true;
        bool auto_switch = false;
        int target_mode = 0; // 0=closest, 1=lowest_hp, 2=looking_at
        int hitbox = 0; // 0=head, 1=body, 2=random
        float max_distance = 50.0f;
        bool visible_check = true;
    };

    // JNI Method IDs cache
    struct JNICache {
        // Minecraft class
        jclass minecraft_class = nullptr;
        jfieldID minecraft_thePlayer = nullptr;
        jfieldID minecraft_theWorld = nullptr;
        jfieldID minecraft_objectMouseOver = nullptr;
        jfieldID minecraft_gameSettings = nullptr;
        jmethodID minecraft_clickMouse = nullptr;
        jmethodID minecraft_rightClickMouse = nullptr;

        // EntityPlayerSP
        jclass entityPlayerSP_class = nullptr;
        jmethodID entityPlayerSP_swingItem = nullptr;
        jmethodID entityPlayerSP_getDistanceToEntity = nullptr;
        jfieldID entityPlayerSP_posX = nullptr;
        jfieldID entityPlayerSP_posY = nullptr;
        jfieldID entityPlayerSP_posZ = nullptr;

        // EntityLivingBase
        jclass entityLivingBase_class = nullptr;
        jmethodID entityLivingBase_getHealth = nullptr;
        jmethodID entityLivingBase_getMaxHealth = nullptr;
        jmethodID entityLivingBase_isDead = nullptr;

        // Entity
        jclass entity_class = nullptr;
        jfieldID entity_posX = nullptr;
        jfieldID entity_posY = nullptr;
        jfieldID entity_posZ = nullptr;
        jfieldID entity_boundingBox = nullptr;

        // AxisAlignedBB
        jclass axisAlignedBB_class = nullptr;
        jfieldID axisAlignedBB_minX = nullptr;
        jfieldID axisAlignedBB_minY = nullptr;
        jfieldID axisAlignedBB_minZ = nullptr;
        jfieldID axisAlignedBB_maxX = nullptr;
        jfieldID axisAlignedBB_maxY = nullptr;
        jfieldID axisAlignedBB_maxZ = nullptr;

        // World
        jclass world_class = nullptr;
        jfieldID world_playerEntities = nullptr;
        jfieldID world_loadedEntityList = nullptr;

        // GameSettings
        jclass gameSettings_class = nullptr;
        jfieldID gameSettings_keyBindAttack = nullptr;
        jfieldID gameSettings_keyBindUseItem = nullptr;

        // KeyBinding
        jclass keyBinding_class = nullptr;
        jmethodID keyBinding_pressKey = nullptr;
        jmethodID keyBinding_releaseKey = nullptr;

        // MovingObjectPosition
        jclass movingObjectPosition_class = nullptr;
        jfieldID movingObjectPosition_typeOfHit = nullptr;
        jfieldID movingObjectPosition_blockPos = nullptr;

        // BlockPos
        jclass blockPos_class = nullptr;
    };

    // Global JNI cache
    extern JNICache g_jniCache;
    extern JavaVM* g_javaVM;
    extern JNIEnv* g_jniEnv;
    extern bool g_initialized;
    extern bool g_shutdownRequested;

} // namespace Nemotron

#endif // NEMOTRON_COMMON_H