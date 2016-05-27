#ifndef HACKLIB_DRAWERD3D_H
#define HACKLIB_DRAWERD3D_H

#include "hacklib/IDrawer.h"
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
    friend class DrawerD3D;
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
    friend class DrawerD3D;
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
    friend class DrawerD3D;
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
    friend class DrawerD3D;
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
    friend class DrawerD3D;
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


class DrawerD3D : public hl::IDrawer
{
public:
    void onLostDevice() override;
    void onResetDevice() override;

    void drawLine(float x1, float y1, float x2, float y2, hl::Color color) const override;
    void drawRect(float x, float y, float w, float h, hl::Color color) const override;
    void drawRectFilled(float x, float y, float w, float h, hl::Color color) const override;
    void drawCircle(float mx, float my, float r, hl::Color color) const override;
    void drawCircleFilled(float mx, float my, float r, hl::Color color) const override;

    const Font *allocFont(std::string fontname, int size, bool bold = true);
    void drawFont(const Font *pFont, float x, float y, D3DCOLOR color, std::string format, va_list valist) const;
    void drawFont(const Font *pFont, float x, float y, D3DCOLOR color, std::string format, ...) const;
    D3DXVECTOR2 textInfo(const Font *pFont, std::string str) const;
    void releaseFont(const Font *pFont);

    const Texture *allocTexture(std::string filename);
    const Texture *allocTexture(const void *buffer, size_t size);
    void drawTexture(const Texture *pTexture, float x, float y, float w, float h) const;
    void releaseTexture(const Texture *pTexture);

    template <class T>
    const VertexBuffer *allocVertexBuffer(const std::vector<T> &vertices);
    const IndexBuffer *allocIndexBuffer(const std::vector<unsigned int> &indices);
    // Draws a primitive. The IndexBuffer is optional. See d3d documentation for primitive types.
    void drawPrimitive(const VertexBuffer *pVertBuf, const IndexBuffer *pIndBuf, D3DPRIMITIVETYPE type, const D3DXMATRIX &worldMatrix) const;
    void drawPrimitive(const VertexBuffer *pVertBuf, const IndexBuffer *pIndBuf, D3DPRIMITIVETYPE type, const D3DXMATRIX &worldMatrix, D3DCOLOR color) const;
    void releaseVertexBuffer(const VertexBuffer *pVertBuf);
    void releaseIndexBuffer(const IndexBuffer *pIndBuf);

    const Sprite *allocSprite();
    void releaseSprite(const Sprite *pSprite);

private:
    template <typename T, typename... Ts>
    const T *alloc(std::vector<std::unique_ptr<T>>& container, Ts&&... vs);
    template <typename T>
    void release(std::vector<std::unique_ptr<T>>& container, const T *instance);

protected:
    void updateDimensions() override;

protected:
    std::vector<std::unique_ptr<Font>> m_fonts;
    std::vector<std::unique_ptr<Texture>> m_textures;
    std::vector<std::unique_ptr<VertexBuffer>> m_vertexBuffers;
    std::vector<std::unique_ptr<IndexBuffer>> m_indexBuffers;
    std::vector<std::unique_ptr<Sprite>> m_sprites;

};


template <typename T, typename... Ts>
const T *DrawerD3D::alloc(std::vector<std::unique_ptr<T>>& container, Ts&&... vs)
{
    auto resource = std::make_unique<T>(std::forward<Ts>(vs)...);

    auto result = resource.get();
    container.push_back(std::move(resource));
    return result;
}


template <typename T>
void DrawerD3D::release(std::vector<std::unique_ptr<T>>& container, const T *instance)
{
    container.erase(std::remove_if(container.begin(), container.end(), [instance](const auto& uptr){
        return uptr.get() == instance;
    }), container.end());
}


template <class T>
const VertexBuffer *DrawerD3D::allocVertexBuffer(const std::vector<T> &vertices)
{
    IDirect3DVertexBuffer9 *pd3dVertexBuffer;
    if (m_context->CreateVertexBuffer(static_cast<UINT>(vertices.size()*sizeof(T)), 0, T::FVF, D3DPOOL_MANAGED, &pd3dVertexBuffer, NULL) != D3D_OK)
        return nullptr;

    T *pData = nullptr;
    pd3dVertexBuffer->Lock(0, 0, reinterpret_cast<void**>(&pData), 0);
    memcpy(pData, vertices.data(), vertices.size()*sizeof(T));
    pd3dVertexBuffer->Unlock();

    return alloc(m_vertexBuffers, T::FVF, pd3dVertexBuffer, static_cast<unsigned int>(vertices.size()));
}

}

#endif
