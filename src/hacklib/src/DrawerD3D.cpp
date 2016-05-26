#include "hacklib/DrawerD3D.h"


using namespace hl;


class TempFVF {
    IDirect3DDevice9 *dev;
    DWORD prevFVF;
public:
    TempFVF(IDirect3DDevice9 *pDevice, DWORD fvf) {
        dev = pDevice;
        dev->GetFVF(&prevFVF);
        dev->SetFVF(fvf);
    }
    ~TempFVF() {
        dev->SetFVF(prevFVF);
    }
};
class TempColor
{
    IDirect3DDevice9 *dev;
    DWORD colorOP, alphaOP, colorArg, alphaArg1, alphaArg2, constant;
public:
    TempColor(IDirect3DDevice9 *pDevice, D3DCOLOR color) {
        dev = pDevice;
        dev->GetTextureStageState(0, D3DTSS_COLOROP, &colorOP);
        dev->GetTextureStageState(0, D3DTSS_ALPHAOP, &alphaOP);
        dev->GetTextureStageState(0, D3DTSS_COLORARG1, &colorArg);
        dev->GetTextureStageState(0, D3DTSS_ALPHAARG1, &alphaArg1);
        dev->GetTextureStageState(0, D3DTSS_ALPHAARG2, &alphaArg2);
        dev->GetTextureStageState(0, D3DTSS_CONSTANT, &constant);
        dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
        dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
        dev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_CONSTANT);
        dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_CONSTANT);
        dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
        dev->SetTextureStageState(0, D3DTSS_CONSTANT, color);
    }
    ~TempColor() {
        dev->SetTextureStageState(0, D3DTSS_COLOROP, colorOP);
        dev->SetTextureStageState(0, D3DTSS_ALPHAOP, alphaOP);
        dev->SetTextureStageState(0, D3DTSS_COLORARG1, colorArg);
        dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, alphaArg1);
        dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, alphaArg2);
        dev->SetTextureStageState(0, D3DTSS_CONSTANT, constant);
    }
};


Font::~Font()
{
    if (m_pFont)
        m_pFont->Release();
}
Texture::~Texture()
{
    if (m_pTexture)
        m_pTexture->Release();
}
VertexBuffer::~VertexBuffer()
{
    if (m_pVertexBuffer)
        m_pVertexBuffer->Release();
}
IndexBuffer::~IndexBuffer()
{
    if (m_pIndexBuffer)
        m_pIndexBuffer->Release();
}
void Sprite::SetTransform(const D3DXMATRIX &transform)
{
    if (m_pSprite)
        m_pSprite->SetTransform(&transform);
}
bool Sprite::Begin()
{
    if (m_pSprite)
        if (m_pSprite->Begin(D3DXSPRITE_ALPHABLEND | D3DXSPRITE_SORT_TEXTURE) == D3D_OK)
            return true;

    return false;
}
void Sprite::End()
{
    if (m_pSprite)
        m_pSprite->End();
}
Sprite::~Sprite()
{
    if (m_pSprite)
        m_pSprite->Release();
}



void DrawerD3D::onLostDevice()
{
    for (auto& i : m_fonts)
        i->m_pFont->OnLostDevice();
    for (auto& i : m_sprites)
        i->m_pSprite->OnLostDevice();
}

void DrawerD3D::onResetDevice()
{
    for (auto& i : m_fonts)
        i->m_pFont->OnResetDevice();
    for (auto& i : m_sprites)
        i->m_pSprite->OnResetDevice();
}


void DrawerD3D::drawLine(float x1, float y1, float x2, float y2, hl::Color color) const
{
    TempFVF fvf(m_pDevice, VERTEX_2D_DIF::FVF);

    VERTEX_2D_DIF verts[] = {
        { x1, y1, 0, 1, color },
        { x2, y2, 0, 1, color }
    };

    m_pDevice->DrawPrimitiveUP(D3DPT_LINELIST, 1, &verts, sizeof(VERTEX_2D_DIF));
}

void DrawerD3D::drawRect(float x, float y, float w, float h, hl::Color color) const
{
    TempFVF fvf(m_pDevice, VERTEX_2D_DIF::FVF);

    VERTEX_2D_DIF verts[] = {
        { x, y, 0, 1, color },
        { x+w, y, 0, 1, color },
        { x+w, y+h, 0, 1, color },
        { x, y+h, 0, 1, color },
        { x, y, 0, 1, color }
    };

    m_pDevice->DrawPrimitiveUP(D3DPT_LINESTRIP, 4, &verts, sizeof(VERTEX_2D_DIF));
}

void DrawerD3D::drawRectFilled(float x, float y, float w, float h, hl::Color color) const
{
    TempFVF fvf(m_pDevice, VERTEX_2D_DIF::FVF);

    VERTEX_2D_DIF verts[] = {
        { x, y+h, 0, 1, color },
        { x, y, 0, 1, color },
        { x+w, y+h, 0, 1, color },
        { x+w, y, 0, 1, color }
    };

    m_pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, &verts, sizeof(VERTEX_2D_DIF));
}

void DrawerD3D::drawCircle(float mx, float my, float r, hl::Color color) const
{
    TempFVF fvf(m_pDevice, VERTEX_2D_DIF::FVF);

    VERTEX_2D_DIF verts[CIRCLE_RESOLUTION+1];

    for (int i = 0; i < CIRCLE_RESOLUTION+1; i++)
    {
        verts[i].x = mx + r * cos(D3DX_PI * (i / (CIRCLE_RESOLUTION / 2.0f)));
        verts[i].y = my + r * sin(D3DX_PI * (i / (CIRCLE_RESOLUTION / 2.0f)));
        verts[i].z = 0;
        verts[i].rhw = 1;
        verts[i].color = color;
    }

    m_pDevice->DrawPrimitiveUP(D3DPT_LINESTRIP, CIRCLE_RESOLUTION, &verts, sizeof(VERTEX_2D_DIF));
}

void DrawerD3D::drawCircleFilled(float mx, float my, float r, hl::Color color) const
{
    TempFVF fvf(m_pDevice, VERTEX_2D_DIF::FVF);

    VERTEX_2D_DIF verts[CIRCLE_RESOLUTION+1];

    for (int i = 0; i < CIRCLE_RESOLUTION+1; i++)
    {
        verts[i].x = mx + r * cos(D3DX_PI * (i / (CIRCLE_RESOLUTION / 2.0f)));
        verts[i].y = my + r * sin(D3DX_PI * (i / (CIRCLE_RESOLUTION / 2.0f)));
        verts[i].z = 0;
        verts[i].rhw = 1;
        verts[i].color = color;
    }

    m_pDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, CIRCLE_RESOLUTION-1, &verts, sizeof(VERTEX_2D_DIF));
}


const Font *DrawerD3D::AllocFont(std::string fontname, int size, bool bold)
{
    ID3DXFont *pFont;
    if (D3DXCreateFont(m_pDevice, size, 0, (bold ? FW_BOLD : FW_NORMAL), 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH|FF_DONTCARE, fontname.c_str(), &pFont) != D3D_OK)
        return nullptr;

    return Alloc(m_fonts, pFont);
}

void DrawerD3D::DrawFont(const Font *pFont, float x, float y, D3DCOLOR color, std::string format, va_list valist) const
{
    va_list valist_copy;
    va_copy(valist_copy, valist);

    int size = vsnprintf(nullptr, 0, format.c_str(), valist);
    char *cStr = new char[size+1];
    vsnprintf(cStr, size+1, format.c_str(), valist_copy);

    RECT rect;
    rect.left = static_cast<LONG>(x);
    rect.top = static_cast<LONG>(y);
    rect.right = 0;
    rect.bottom = 0;
    pFont->m_pFont->DrawText(NULL, cStr, size, &rect, DT_NOCLIP, color);

    delete[] cStr;
    va_end(valist_copy);
}

void DrawerD3D::DrawFont(const Font *pFont, float x, float y, D3DCOLOR color, std::string format, ...) const
{
    va_list vl;
    va_start(vl, format);
    DrawFont(pFont, x, y, color, format, vl);
    va_end(vl);
}

D3DXVECTOR2 DrawerD3D::TextInfo(const Font *pFont, std::string str) const {
    D3DXVECTOR2 ret = { 0, 0 };
    RECT rect = { 0, 0, 0, 0 };
    pFont->m_pFont->DrawText(NULL, str.c_str(), -1, &rect, DT_CALCRECT, 0);
    ret.x = float(rect.right - rect.left);
    ret.y = float(rect.bottom - rect.top);
    return ret;
}

void DrawerD3D::ReleaseFont(const Font *pFont)
{
    Release(m_fonts, pFont);
}


const Texture *DrawerD3D::AllocTexture(std::string filename)
{
    IDirect3DTexture9 *pTexture;
    if (D3DXCreateTextureFromFile(m_pDevice, filename.c_str(), &pTexture) != D3D_OK)
        return nullptr;

    return Alloc(m_textures, pTexture);
}

const Texture *DrawerD3D::AllocTexture(const void *buffer, size_t size)
{
    IDirect3DTexture9 *pTexture;
    if (D3DXCreateTextureFromFileInMemory(m_pDevice, buffer, (UINT)size, &pTexture) != D3D_OK)
        return nullptr;

    return Alloc(m_textures, pTexture);
}

void DrawerD3D::DrawTexture(const Texture *pTexture, float x, float y, float w, float h) const
{
    TempFVF fvf(m_pDevice, VERTEX_2D_TEX::FVF);

    VERTEX_2D_TEX verts[] = {
        { x, y+h, 0, 1, 0, 1 },
        { x, y, 0, 1, 0, 0 },
        { x+w, y+h, 0, 1, 1, 1 },
        { x+w, y, 0, 1, 1, 0 }
    };

    IDirect3DBaseTexture9 *oldTexture;
    m_pDevice->GetTexture(0, &oldTexture);
    m_pDevice->SetTexture(0, pTexture->m_pTexture);
    m_pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, &verts, sizeof(VERTEX_2D_TEX));
    m_pDevice->SetTexture(0, oldTexture);
}

void DrawerD3D::ReleaseTexture(const Texture *pTexture)
{
    Release(m_textures, pTexture);
}


const IndexBuffer *DrawerD3D::AllocIndexBuffer(const std::vector<unsigned int> &indices)
{
    bool is4byte = indices.size() > USHRT_MAX;

    IDirect3DIndexBuffer9 *pd3dIndexBuffer;
    if (m_pDevice->CreateIndexBuffer(static_cast<UINT>(indices.size()*(is4byte ? sizeof(unsigned int) : sizeof(unsigned short))), 0, is4byte ? D3DFMT_INDEX32 : D3DFMT_INDEX16, D3DPOOL_MANAGED, &pd3dIndexBuffer, NULL) != D3D_OK)
        return nullptr;

    if (is4byte) {
        unsigned int *pData = nullptr;
        pd3dIndexBuffer->Lock(0, 0, reinterpret_cast<void**>(&pData), 0);
        memcpy(pData, indices.data(), indices.size()*sizeof(unsigned int));
        pd3dIndexBuffer->Unlock();
    } else {
        unsigned short *pData = nullptr;
        pd3dIndexBuffer->Lock(0, 0, reinterpret_cast<void**>(&pData), 0);
        for (size_t i = 0; i < indices.size(); i++)
            pData[i] = indices[i];
        pd3dIndexBuffer->Unlock();
    }

    return Alloc(m_indexBuffers, is4byte ? D3DFMT_INDEX32 : D3DFMT_INDEX16, pd3dIndexBuffer, static_cast<unsigned int>(indices.size()));
}

void DrawerD3D::DrawPrimitive(const VertexBuffer *pVertBuf, const IndexBuffer *pIndBuf, D3DPRIMITIVETYPE type, const D3DXMATRIX &worldMatrix) const
{
    TempFVF fvf(m_pDevice, pVertBuf->m_format);

    unsigned int vertexSize = 0;
    switch (pVertBuf->m_format) {
    case VERTEX_2D_COL::FVF: vertexSize = sizeof(VERTEX_2D_COL); break;
    case VERTEX_2D_DIF::FVF: vertexSize = sizeof(VERTEX_2D_DIF); break;
    case VERTEX_2D_TEX::FVF: vertexSize = sizeof(VERTEX_2D_TEX); break;
    case VERTEX_3D_COL::FVF: vertexSize = sizeof(VERTEX_3D_COL); break;
    case VERTEX_3D_DIF::FVF: vertexSize = sizeof(VERTEX_3D_DIF); break;
    default: return;
    }

    unsigned int numVertices = pIndBuf ? pIndBuf->m_numIndices : pVertBuf->m_numVertices;
    unsigned int numPrimitives = 0;
    switch (type) {
    case D3DPT_POINTLIST:       numPrimitives = numVertices;    break;
    case D3DPT_LINELIST:        numPrimitives = numVertices/2;  break;
    case D3DPT_LINESTRIP:       numPrimitives = numVertices-1;  break;
    case D3DPT_TRIANGLELIST:    numPrimitives = numVertices/3;  break;
    case D3DPT_TRIANGLESTRIP:
    case D3DPT_TRIANGLEFAN:     numPrimitives = numVertices-2;  break;
    }


    m_pDevice->SetTransform(D3DTS_WORLD, &worldMatrix);
    m_pDevice->SetTransform(D3DTS_VIEW, &m_viewMatrix);
    m_pDevice->SetTransform(D3DTS_PROJECTION, &m_projMatrix);
    m_pDevice->SetStreamSource(0, pVertBuf->m_pVertexBuffer, 0, vertexSize);
    if (pIndBuf) {
        m_pDevice->SetIndices(pIndBuf->m_pIndexBuffer);
        m_pDevice->DrawIndexedPrimitive(type, 0, 0, pVertBuf->m_numVertices, 0, numPrimitives);
    } else {
        m_pDevice->DrawPrimitive(type, 0, numPrimitives);
    }
}

void DrawerD3D::DrawPrimitive(const VertexBuffer *pVertBuf, const IndexBuffer *pIndBuf, D3DPRIMITIVETYPE type, const D3DXMATRIX &worldMatrix, D3DCOLOR color) const
{
    TempColor col(m_pDevice, color);
    DrawPrimitive(pVertBuf, pIndBuf, type, worldMatrix);
}

void DrawerD3D::ReleaseVertexBuffer(const VertexBuffer *pVertBuf)
{
    Release(m_vertexBuffers, pVertBuf);
}

void DrawerD3D::ReleaseIndexBuffer(const IndexBuffer *pIndBuf)
{
    Release(m_indexBuffers, pIndBuf);
}


const Sprite *DrawerD3D::AllocSprite()
{
    ID3DXSprite *pSprite;
    if (D3DXCreateSprite(m_pDevice, &pSprite) != D3D_OK)
        return nullptr;

    return Alloc(m_sprites, pSprite);
}

void DrawerD3D::ReleaseSprite(const Sprite *pSprite)
{
    Release(m_sprites, pSprite);
}


void DrawerD3D::updateDimensions()
{
    D3DVIEWPORT9 viewport;
    m_context->GetViewport(&viewport);
    m_width = static_cast<float>(viewport.Width);
    m_height = static_cast<float>(viewport.Height);
}
