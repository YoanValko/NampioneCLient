#include "jni_helper.h"
#include <cmath>
#include <cstdlib>
#include <ctime>

namespace Nemotron {

    static JavaVM* g_javaVM = nullptr;
    static JNIEnv* g_jniEnv = nullptr;

    JNIEnv* JNIHelper::GetJNIEnv() {
        return g_jniEnv;
    }

    void JNIHelper::CacheJNIEnvironment(JNIEnv* env) {
        g_jniEnv = env;
    }

    bool JNIHelper::InitializeJNICache(JNIEnv* env) {
        CacheJNIEnvironment(env);
        
        // Cache Minecraft class
        g_jniCache.minecraft_class = FindClass(env, "net/minecraft/client/Minecraft");
        if (!g_jniCache.minecraft_class) return false;
        
        g_jniCache.minecraft_thePlayer = GetFieldID(env, g_jniCache.minecraft_class, "thePlayer", "Lnet/minecraft/client/entity/EntityPlayerSP;");
        g_jniCache.minecraft_theWorld = GetFieldID(env, g_jniCache.minecraft_class, "theWorld", "Lnet/minecraft/client/multiplayer/WorldClient;");
        g_jniCache.minecraft_objectMouseOver = GetFieldID(env, g_jniCache.minecraft_class, "objectMouseOver", "Lnet/minecraft/util/MovingObjectPosition;");
        g_jniCache.minecraft_gameSettings = GetFieldID(env, g_jniCache.minecraft_class, "gameSettings", "Lnet/minecraft/client/settings/GameSettings;");
        g_jniCache.minecraft_clickMouse = GetMethodID(env, g_jniCache.minecraft_class, "clickMouse", "()V");
        g_jniCache.minecraft_rightClickMouse = GetMethodID(env, g_jniCache.minecraft_class, "rightClickMouse", "()V");
        
        // EntityPlayerSP
        g_jniCache.entityPlayerSP_class = FindClass(env, "net/minecraft/client/entity/EntityPlayerSP");
        if (!g_jniCache.entityPlayerSP_class) return false;
        
        g_jniCache.entityPlayerSP_swingItem = GetMethodID(env, g_jniCache.entityPlayerSP_class, "swingItem", "()V");
        g_jniCache.entityPlayerSP_getDistanceToEntity = GetMethodID(env, g_jniCache.entityPlayerSP_class, "getDistanceToEntity", "(Lnet/minecraft/entity/Entity;)D");
        g_jniCache.entityPlayerSP_posX = GetFieldID(env, g_jniCache.entityPlayerSP_class, "posX", "D");
        g_jniCache.entityPlayerSP_posY = GetFieldID(env, g_jniCache.entityPlayerSP_class, "posY", "D");
        g_jniCache.entityPlayerSP_posZ = GetFieldID(env, g_jniCache.entityPlayerSP_class, "posZ", "D");
        
        // EntityLivingBase
        g_jniCache.entityLivingBase_class = FindClass(env, "net/minecraft/entity/EntityLivingBase");
        if (!g_jniCache.entityLivingBase_class) return false;
        
        g_jniCache.entityLivingBase_getHealth = GetMethodID(env, g_jniCache.entityLivingBase_class, "getHealth", "()F");
        g_jniCache.entityLivingBase_getMaxHealth = GetMethodID(env, g_jniCache.entityLivingBase_class, "getMaxHealth", "()F");
        g_jniCache.entityLivingBase_isDead = GetMethodID(env, g_jniCache.entityLivingBase_class, "isDead", "()Z");
        
        // Entity
        g_jniCache.entity_class = FindClass(env, "net/minecraft/entity/Entity");
        if (!g_jniCache.entity_class) return false;
        
        g_jniCache.entity_posX = GetFieldID(env, g_jniCache.entity_class, "posX", "D");
        g_jniCache.entity_posY = GetFieldID(env, g_jniCache.entity_class, "posY", "D");
        g_jniCache.entity_posZ = GetFieldID(env, g_jniCache.entity_class, "posZ", "D");
        g_jniCache.entity_boundingBox = GetFieldID(env, g_jniCache.entity_class, "boundingBox", "Lnet/minecraft/util/AxisAlignedBB;");
        
        // AxisAlignedBB
        g_jniCache.axisAlignedBB_class = FindClass(env, "net/minecraft/util/AxisAlignedBB");
        if (!g_jniCache.axisAlignedBB_class) return false;
        
        g_jniCache.axisAlignedBB_minX = GetFieldID(env, g_jniCache.axisAlignedBB_class, "minX", "D");
        g_jniCache.axisAlignedBB_minY = GetFieldID(env, g_jniCache.axisAlignedBB_class, "minY", "D");
        g_jniCache.axisAlignedBB_minZ = GetFieldID(env, g_jniCache.axisAlignedBB_class, "minZ", "D");
        g_jniCache.axisAlignedBB_maxX = GetFieldID(env, g_jniCache.axisAlignedBB_class, "maxX", "D");
        g_jniCache.axisAlignedBB_maxY = GetFieldID(env, g_jniCache.axisAlignedBB_class, "maxY", "D");
        g_jniCache.axisAlignedBB_maxZ = GetFieldID(env, g_jniCache.axisAlignedBB_class, "maxZ", "D");
        
        // World
        g_jniCache.world_class = FindClass(env, "net/minecraft/client/multiplayer/WorldClient");
        if (!g_jniCache.world_class) return false;
        
        g_jniCache.world_playerEntities = GetFieldID(env, g_jniCache.world_class, "playerEntities", "Ljava/util/List;");
        g_jniCache.world_loadedEntityList = GetFieldID(env, g_jniCache.world_class, "loadedEntityList", "Ljava/util/List;");
        
        // GameSettings
        g_jniCache.gameSettings_class = FindClass(env, "net/minecraft/client/settings/GameSettings");
        if (!g_jniCache.gameSettings_class) return false;
        
        g_jniCache.gameSettings_keyBindAttack = GetFieldID(env, g_jniCache.gameSettings_class, "keyBindAttack", "Lnet/minecraft/client/settings/KeyBinding;");
        g_jniCache.gameSettings_keyBindUseItem = GetFieldID(env, g_jniCache.gameSettings_class, "keyBindUseItem", "Lnet/minecraft/client/settings/KeyBinding;");
        
        // KeyBinding
        g_jniCache.keyBinding_class = FindClass(env, "net/minecraft/client/settings/KeyBinding");
        if (!g_jniCache.keyBinding_class) return false;
        
        g_jniCache.keyBinding_pressKey = GetMethodID(env, g_jniCache.keyBinding_class, "pressKey", "()V");
        g_jniCache.keyBinding_releaseKey = GetMethodID(env, g_jniCache.keyBinding_class, "releaseKey", "()V");
        
        // MovingObjectPosition
        g_jniCache.movingObjectPosition_class = FindClass(env, "net/minecraft/util/MovingObjectPosition");
        if (!g_jniCache.movingObjectPosition_class) return false;
        
        g_jniCache.movingObjectPosition_typeOfHit = GetFieldID(env, g_jniCache.movingObjectPosition_class, "typeOfHit", "Lnet/minecraft/util/MovingObjectPosition$MovingObjectType;");
        g_jniCache.movingObjectPosition_blockPos = GetFieldID(env, g_jniCache.movingObjectPosition_class, "blockPos", "Lnet/minecraft/util/BlockPos;");
        
        // BlockPos
        g_jniCache.blockPos_class = FindClass(env, "net/minecraft/util/BlockPos");
        
        return true;
    }

    jclass JNIHelper::FindClass(JNIEnv* env, const char* name) {
        jclass clazz = env->FindClass(name);
        CheckAndClearException(env);
        return clazz;
    }

    jclass JNIHelper::GetClass(JNIEnv* env, const char* name) {
        return FindClass(env, name);
    }

    jfieldID JNIHelper::GetFieldID(JNIEnv* env, jclass clazz, const char* name, const char* sig) {
        jfieldID field = env->GetFieldID(clazz, name, sig);
        CheckAndClearException(env);
        return field;
    }

    jmethodID JNIHelper::GetMethodID(JNIEnv* env, jclass clazz, const char* name, const char* sig) {
        jmethodID method = env->GetMethodID(clazz, name, sig);
        CheckAndClearException(env);
        return method;
    }

    jmethodID JNIHelper::GetStaticMethodID(JNIEnv* env, jclass clazz, const char* name, const char* sig) {
        jmethodID method = env->GetStaticMethodID(clazz, name, sig);
        CheckAndClearException(env);
        return method;
    }

    void JNIHelper::CheckAndClearException(JNIEnv* env) {
        if (env->ExceptionCheck()) {
            env->ExceptionDescribe();
            env->ExceptionClear();
        }
    }

    void JNIHelper::ThrowException(JNIEnv* env, const char* className, const char* message) {
        jclass exceptionClass = env->FindClass(className);
        if (exceptionClass) {
            env->ThrowNew(exceptionClass, message);
        }
    }

    jobject JNIHelper::NewObject(JNIEnv* env, jclass clazz, jmethodID methodID, ...) {
        va_list args;
        va_start(args, methodID);
        jobject obj = env->NewObjectV(clazz, methodID, args);
        va_end(args);
        CheckAndClearException(env);
        return obj;
    }

    jobject JNIHelper::NewObjectA(JNIEnv* env, jclass clazz, jmethodID methodID, jvalue* args) {
        jobject obj = env->NewObjectA(clazz, methodID, args);
        CheckAndClearException(env);
        return obj;
    }

    // Math helpers implementation
    Rotation CalculateAngle(const Vec3& from, const Vec3& to) {
        Vec3 delta = to - from;
        float yaw = atan2f(delta.z, delta.x) * 180.0f / M_PI - 90.0f;
        float pitch = atan2f(-delta.y, delta.Length2D()) * 180.0f / M_PI;
        return Rotation(yaw, pitch);
    }

    void NormalizeAngles(Rotation& rot) {
        while (rot.yaw > 180.0f) rot.yaw -= 360.0f;
        while (rot.yaw < -180.0f) rot.yaw += 360.0f;
        while (rot.pitch > 89.0f) rot.pitch = 89.0f;
        while (rot.pitch < -89.0f) rot.pitch = -89.0f;
    }

    Rotation SmoothRotation(const Rotation& current, const Rotation& target, float smoothFactor) {
        Rotation diff = Rotation(target.yaw - current.yaw, target.pitch - current.pitch);
        NormalizeAngles(diff);
        return Rotation(current.yaw + diff.yaw / smoothFactor, current.pitch + diff.pitch / smoothFactor);
    }

    float Lerp(float a, float b, float t) {
        return a + (b - a) * t;
    }

    Vec3 Lerp(const Vec3& a, const Vec3& b, float t) {
        return a + (b - a) * t;
    }

    float RandomFloat(float min, float max) {
        return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
    }

    int RandomInt(int min, int max) {
        return min + rand() % (max - min + 1);
    }

    Vec3 RandomVec3(float radius) {
        float x = RandomFloat(-radius, radius);
        float y = RandomFloat(-radius, radius);
        float z = RandomFloat(-radius, radius);
        return Vec3(x, y, z);
    }

    // Initialize random seed
    void InitRandom() {
        srand(static_cast<unsigned int>(time(nullptr)));
    }

} // namespace Nemotron