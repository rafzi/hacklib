#ifndef HACKLIB_IDRAWER_H
#define HACKLIB_IDRAWER_H

#include "hacklib/Handles.h"
#include "glm/glm.hpp"


namespace hl {


class Vec3 : public glm::vec3
{
public:
    using glm::vec3::vec3;

    //operator D3DXVECTOR3() const
};

class Mat4x4 : public glm::mat4x4
{
public:
    using glm::mat4x4::mat4x4;
};

// Represents an A8R8G8B8 color.
class Color
{
public:
    Color();
    Color(uint8_t red, uint8_t green, uint8_t blue);
    Color(uint8_t alpha, uint8_t red, uint8_t green, uint8_t blue);
    Color(uint32_t combined);

    operator uint32_t() const
    {
        return m_data;
    }
    void glSet() const;

private:
    uint32_t m_data;
};


class IDrawer
{
public:
    // Releases all resources aquired by the Alloc* functions.
    virtual void clearRessources();

    // Associates a graphics GraphicsContext with the drawer. Must be done before
    // calling any other function.
    void setContext(hl::GraphicsContext context);
    hl::GraphicsContext getContext() const;

    // Must be called before the graphics context is reset.
    virtual void onLostDevice();
    // Must be called after the graphics context is reset.
    virtual void onResetDevice();

    // Changes the view and projection matrix for 3D drawing.
    void update(const hl::Mat4x4& viewMatrix, const hl::Mat4x4& projectionMatrix);

    // Gets the width and height of the viewport.
    float getWidth() const;
    float getHeight() const;

    // Projects a position in world coordiantes to screen coordinates.
    void project(const hl::Vec3& worldPos, hl::Vec3& screenPos, const hl::Mat4x4 *worldMatrix = nullptr) const;
    // Returns true if the screen position is in front of the camera.
    virtual bool isInfrontCam(const hl::Vec3& screenPos) const;
    // Checks if a screen position would be visible on the viewport. The offScreenTolerance is measured in pixels.
    // Returns true if the position is behind the camera, but on the viewport.
    bool isOnScreen(const hl::Vec3& screenPos, float offScreenTolerance = 0) const;

    virtual void drawLine(float x1, float y1, float x2, float y2, hl::Color color) const;
    void drawLineProjected(hl::Vec3 pos1, hl::Vec3 pos2, hl::Color color) const;
    virtual void drawRect(float x, float y, float w, float h, hl::Color color) const;
    virtual void drawRectFilled(float x, float y, float w, float h, hl::Color color) const;
    virtual void drawCircle(float mx, float my, float r, hl::Color color) const;
    virtual void drawCircleFilled(float mx, float my, float r, hl::Color color) const;

protected:
    // Implement this to set the members m_width and m_height. Is called from update().
    virtual void updateDimensions() = 0;

protected:
    static const int CIRCLE_RESOLUTION = 64;

    hl::GraphicsContext m_context = nullptr;

    float m_width = 0;
    float m_height = 0;
    hl::Mat4x4 m_viewMatrix;
    hl::Mat4x4 m_projMatrix;

};

}

#endif
