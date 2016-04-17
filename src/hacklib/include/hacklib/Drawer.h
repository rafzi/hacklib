#ifndef HACKLIB_DRAWER_H
#define HACKLIB_DRAWER_H

// link with: d3d9.lib d3dx9.lib
#include "d3d9.h"
#include "d3dx9.h"
#include <vector>
#include <string>
#include <memory>
#include <algorithm>


namespace hl {


struct VERTEX_2D_COL { // transformed colorizable
    float x, y, z, rhw;
    static const DWORD FVF = D3DFVF_XYZRHW;
};
struct VERTEX_2D_DIF { // transformed colorized
    float x, y, z, rhw;
    D3DCOLOR color;
    static const DWORD FVF = D3DFVF_XYZRHW|D3DFVF_DIFFUSE;
};
struct VERTEX_2D_TEX {
    float x, y, z, rhw;
    float u, v;
    static const DWORD FVF = D3DFVF_XYZRHW|D3DFVF_TEX1;
};
struct VERTEX_3D_COL { // transformable colorizable
    float x, y, z;
    static const DWORD FVF = D3DFVF_XYZ;
};
struct VERTEX_3D_DIF { // transformable colorzed
    float x, y, z;
    D3DCOLOR color;
    static const DWORD FVF = D3DFVF_XYZ|D3DFVF_DIFFUSE;
};


class Font
{
    friend class Drawer;
public:
    Font(ID3DXFont *d3dobj) :
        m_pFont(d3dobj)
    { }
    ~Font();
private:
    ID3DXFont *m_pFont;
};
class Texture
{
    friend class Drawer;
public:
    Texture(IDirect3DTexture9 *d3dobj) :
        m_pTexture(d3dobj)
    { }
    ~Texture();
private:
    IDirect3DTexture9 *m_pTexture;
};
class VertexBuffer
{
    friend class Drawer;
public:
    VertexBuffer(DWORD format, IDirect3DVertexBuffer9 *d3dobj, unsigned int verts) :
        m_format(format),
        m_pVertexBuffer(d3dobj),
        m_numVertices(verts)
    { }
    ~VertexBuffer();
private:
    DWORD m_format;
    IDirect3DVertexBuffer9 *m_pVertexBuffer;
    unsigned int m_numVertices;
};
class IndexBuffer
{
    friend class Drawer;
public:
    IndexBuffer(DWORD format, IDirect3DIndexBuffer9 *d3dobj, unsigned int indices) :
        m_format(format),
        m_pIndexBuffer(d3dobj),
        m_numIndices(indices)
    { }
    ~IndexBuffer();
private:
    DWORD m_format;
    IDirect3DIndexBuffer9 *m_pIndexBuffer;
    unsigned int m_numIndices;
};
class Sprite
{
    friend class Drawer;
public:
    Sprite(ID3DXSprite *d3dobj) :
        m_pSprite(d3dobj)
    { }
    void SetTransform(const D3DXMATRIX &transform);
    bool Begin();
    void End();
    ~Sprite();
private:
    ID3DXSprite *m_pSprite;
};


class Drawer
{
public:
    // Releases all resources aquired by the Alloc* functions.
    void ClearRessources();

    // Associates a d3d device with the drawer. Must be done before
    // calling any other function.
    void SetDevice(IDirect3DDevice9 *pDevice);
    IDirect3DDevice9 *GetDevice() const;

    // Changes the view and projection matrix for 3D drawing.
    void Update(const D3DXMATRIX &viewMatrix, const D3DXMATRIX &projectionMatrix);

    // Gets the width and height of the viewport.
    float GetWidth() const;
    float GetHeight() const;

    // Projects a position in world coordiantes to screen coordinates.
    void Project(const D3DXVECTOR3 &worldPos, D3DXVECTOR3 &screenPos, const D3DXMATRIX *worldMatrix = nullptr) const;
    // Returns true if the screen position is in front of the camera.
    virtual bool IsInfrontCam(const D3DXVECTOR3 &screenPos) const;
    // Checks if a screen position would be visible on the viewport. The offScreenTolerance is measured in pixels.
    // Returns true if the position is behind the camera, but on the viewport.
    bool IsOnScreen(const D3DXVECTOR3 &screenPos, float offScreenTolerance = 0) const;

    void DrawLine(float x1, float y1, float x2, float y2, D3DCOLOR color) const;
    void DrawLineProjected(D3DXVECTOR3 pos1, D3DXVECTOR3 pos2, D3DCOLOR color) const;
    void DrawRect(float x, float y, float w, float h, D3DCOLOR color) const;
    void DrawRectFilled(float x, float y, float w, float h, D3DCOLOR color) const;
    void DrawCircle(float mx, float my, float r, D3DCOLOR color) const;
    void DrawCircleFilled(float mx, float my, float r, D3DCOLOR color) const;

    const Font *AllocFont(std::string fontname, int size, bool bold = true);
    void DrawFont(const Font *pFont, float x, float y, D3DCOLOR color, std::string format, va_list valist) const;
    void DrawFont(const Font *pFont, float x, float y, D3DCOLOR color, std::string format, ...) const;
    D3DXVECTOR2 TextInfo(const Font *pFont, std::string str) const;
    void ReleaseFont(const Font *pFont);

    const Texture *AllocTexture(std::string filename);
    const Texture *AllocTexture(const void *buffer, size_t size);
    void DrawTexture(const Texture *pTexture, float x, float y, float w, float h) const;
    void ReleaseTexture(const Texture *pTexture);

    template <class T>
    const VertexBuffer *AllocVertexBuffer(const std::vector<T> &vertices);
    const IndexBuffer *AllocIndexBuffer(const std::vector<unsigned int> &indices);
    // Draws a primitive. The IndexBuffer is optional. See d3d documentation for primitive types.
    void DrawPrimitive(const VertexBuffer *pVertBuf, const IndexBuffer *pIndBuf, D3DPRIMITIVETYPE type, const D3DXMATRIX &worldMatrix) const;
    void DrawPrimitive(const VertexBuffer *pVertBuf, const IndexBuffer *pIndBuf, D3DPRIMITIVETYPE type, const D3DXMATRIX &worldMatrix, D3DCOLOR color) const;
    void ReleaseVertexBuffer(const VertexBuffer *pVertBuf);
    void ReleaseIndexBuffer(const IndexBuffer *pIndBuf);

    const Sprite *AllocSprite();
    void ReleaseSprite(const Sprite *pSprite);

    // Must be called before Reset.
    void OnLostDevice();
    // Must be called after Reset.
    void OnResetDevice();

private:
    template <typename T, typename... Ts>
    const T *Alloc(std::vector<std::unique_ptr<T>>& container, Ts&&... vs);
    template <typename T>
    void Release(std::vector<std::unique_ptr<T>>& container, const T *instance);

private:
    static const int CIRCLE_RESOLUTION = 64;

    IDirect3DDevice9 *m_pDevice = nullptr;

    D3DVIEWPORT9 m_viewport;
    D3DXMATRIX m_viewMatrix;
    D3DXMATRIX m_projMatrix;

    std::vector<std::unique_ptr<Font>> m_fonts;
    std::vector<std::unique_ptr<Texture>> m_textures;
    std::vector<std::unique_ptr<VertexBuffer>> m_vertexBuffers;
    std::vector<std::unique_ptr<IndexBuffer>> m_indexBuffers;
    std::vector<std::unique_ptr<Sprite>> m_sprites;

};


template <typename T, typename... Ts>
const T *Drawer::Alloc(std::vector<std::unique_ptr<T>>& container, Ts&&... vs)
{
    auto resource = std::make_unique<T>(std::forward<Ts>(vs)...);

    auto result = resource.get();
    container.push_back(std::move(resource));
    return result;
}


template <typename T>
void Drawer::Release(std::vector<std::unique_ptr<T>>& container, const T *instance)
{
    container.erase(std::remove_if(container.begin(), container.end(), [instance](const auto& uptr){
        return uptr.get() == instance;
    }), container.end());
}


template <class T>
const VertexBuffer *Drawer::AllocVertexBuffer(const std::vector<T> &vertices)
{
    IDirect3DVertexBuffer9 *pd3dVertexBuffer;
    if (m_pDevice->CreateVertexBuffer(static_cast<UINT>(vertices.size()*sizeof(T)), 0, T::FVF, D3DPOOL_MANAGED, &pd3dVertexBuffer, NULL) != D3D_OK)
        return nullptr;

    T *pData = nullptr;
    pd3dVertexBuffer->Lock(0, 0, reinterpret_cast<void**>(&pData), 0);
    memcpy(pData, vertices.data(), vertices.size()*sizeof(T));
    pd3dVertexBuffer->Unlock();

    return Alloc(m_vertexBuffers, T::FVF, pd3dVertexBuffer, static_cast<unsigned int>(vertices.size()));
}

}

#endif
