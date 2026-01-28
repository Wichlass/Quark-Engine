#pragma once

class LLCSAPI
{
public:
    virtual ~LLCSAPI() = default;
    
    // ==================== LIFECYCLE ====================
    virtual void init() = 0;
    virtual void shutdown() = 0;
    
    // ==================== FRAME UPDATE ====================
    virtual void update(float deltaTime) = 0;
    
    // ==================== DEBUG ====================
    virtual void debugDraw() = 0;
};