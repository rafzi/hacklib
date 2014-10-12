#include "hacklib/Main.h"
#include "hacklib/PatternScanner.h"
#include "hacklib/D3DDeviceFetcher.h"
#include "hacklib/Hooker.h"
#include "hacklib/ImplementMember.h"
#include "hacklib/Drawer.h"
#include "hacklib/ForeignClass.h"
#include <mutex>
#include <thread>
#include <chrono>


#define HL_LOG(format, ...) ;


void __fastcall hkGameThread(uintptr_t, int, int);
HRESULT __stdcall hkPresent(LPDIRECT3DDEVICE9 pDevice, RECT*, RECT*, HWND, RGNDATA*);
HRESULT __stdcall hkReset(LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS*);


namespace GW2
{
    namespace ANet
    {
        template <typename T>
        class SmartPtr {
        public:
            T *GetPtr() {
                return m_ptr;
            }
        private:
            T *m_ptr;
        };


        template <typename T>
        class Array {
        public:
            Array<T> &operator= (const Array<T> &a) {
                if (this != &a) {
                    m_basePtr = a.m_basePtr;
                    m_capacity = a.m_capacity;
                    m_count = a.m_count;
                }
                return *this;
            }
            T &operator[] (int index) {
                if (index < Count()) {
                    return m_basePtr[index];
                }
                throw 1;
            }
            bool IsValid() {
                if (m_basePtr) return true;
                return false;
            }
            int Count() {
                return m_count;
            }
        private:
            T *m_basePtr;
            int m_capacity;
            int m_count;
        };

        template <typename T>
        class List {
        public:
            void Init(int linkOffset) {
                m_linkOffset = linkOffset;
                m_begin.prev = &m_begin;
                m_begin.head = static_cast<T*>(static_cast<int>(&m_begin) - linkOffset + 1);
            }

            void Iterate() {
                T *node = m_link.head;

                if (node && !TermCheck(node)) {
                    while (node) {
                        int linkObject = static_cast<int>(node)+m_linkOffset;

                        node = *static_cast<T**>(linkObject + 4);
                        if (TermCheck(node)) break;
                    }
                }
            }

            T *Next(T *current) {
                if (current == NULL) {
                    if (m_begin.head && !TermCheck(m_begin.head)) {
                        return m_begin.head;
                    }
                    return NULL;
                }
                T *next = *reinterpret_cast<T**>(reinterpret_cast<int>(current)+m_linkOffset + 4);
                if (next && !TermCheck(next)) {
                    return next;
                }
                return NULL;
            }

        private:
            static bool TermCheck(T *ptr) {
                return (reinterpret_cast<int>(ptr)& 1) == 1;
            }


            int m_linkOffset;
            struct iter {
                iter *prev;
                T *head;
            } m_begin;

            //LINK_OFFSET_UNINIT = 0xdddddddd
            //LIST_LINK_AFTER = 1 / LIST_LINK_BEFORE = 2
        };
    }

    enum Profession {
        PROFESSION_GUARDIAN = 1,
        PROFESSION_WARRIOR,
        PROFESSION_ENGINEER,
        PROFESSION_RANGER,
        PROFESSION_THIEF,
        PROFESSION_ELEMENTALIST,
        PROFESSION_MESMER,
        PROFESSION_NECROMANCER,
        PROFESSION_NONE
    };
    enum Attitude {
        ATTITUDE_FRIENDLY = 0,
        ATTITUDE_HOSTILE,
        ATTITUDE_INDIFFERENT,
        ATTITUDE_NEUTRAL
    };
    enum AgentCategory {
        AGENT_CATEGORY_CHAR = 0,
        AGENT_CATEGORY_DYNAMIC,
        AGENT_CATEGORY_KEYFRAMED
    };
    enum AgentType {
        AGENT_TYPE_CHAR = 0,
        AGENT_TYPE_GADGET = 9,
        AGENT_TYPE_GADGET_ATTACK_TARGET = 10,
        AGENT_TYPE_ITEM = 14
    };

    namespace Agent
    {
        class CAgentTransform
        {
            IMPLMEMBER(float, X, 0x20);
            IMPLMEMBER_REL(float, Y, 0, X);
            IMPLMEMBER_REL(float, Z, 0, Y);
            IMPLMEMBER(float, RX, 0x100);
            IMPLMEMBER(float, RY, 0x104);
            IMPLMEMBER(float, SyncX, 0x110);
            IMPLMEMBER_REL(float, SyncY, 0, SyncX);
            IMPLMEMBER_REL(float, SyncZ, 0, SyncY);
        };

        class CAgentBase
        {
            IMPLVTFUNC(AgentCategory, GetCategory, 0x10);
            IMPLVTFUNC(int, GetAgentId, 0x5c);
            IMPLVTFUNC(AgentType, GetType, 0x9c);
            IMPLVTFUNC(void, GetPos, 0xb4, D3DXVECTOR3*, pPos);

            IMPLMEMBER(CAgentTransform*, Transform, 0x1c);
        };
    }

    namespace CharClient
    {
        class CInventory {
            IMPLMEMBER(int, Supply, 0x1d8);
        };

        class CEndurance {
            IMPLMEMBER(int, CurrentValue, 0x4);
            IMPLMEMBER(int, MaxValue, 0x8);
        };

        class CHealth {
            IMPLMEMBER(float, CurrentValue, 0x8);
            IMPLMEMBER(float, MaxValue, 0xc);
        };

        class CCoreStats {
            IMPLMEMBER(int, Level, 0x184);
            IMPLMEMBER(int, ScaledLevel, 0x1ac);
            IMPLMEMBER(Profession, Prof, 0x1ec);
        };

        class CPlayer {
            IMPLMEMBER(char*, Name, 0x30);
        };

        class CCharacter {
            IMPLVTFUNC(Agent::CAgentBase*, GetAgent, 0x90);
            IMPLVTFUNC(int, GetAgentId, 0x94);
            IMPLVTFUNC(CPlayer*, GetPlayer, 0xcc);
            IMPLVTFUNC(bool, IsAlive, 0x10c);
            IMPLVTFUNC(bool, IsControlled, 0x110);
            IMPLVTFUNC(bool, IsDowned, 0x11c);
            IMPLVTFUNC(bool, IsInWater, 0x13c);
            IMPLVTFUNC(bool, IsMonster, 0x14c);
            IMPLVTFUNC(bool, IsMonsterPlayerClone, 0x15c);
            IMPLVTFUNC(bool, IsPlayer, 0x178);

            IMPLMEMBER(Attitude, AttitudeTowardControlled, 0x58);
            IMPLMEMBER(CCoreStats*, CoreStats, 0x148);
            IMPLMEMBER(CEndurance*, Endurance, 0x168);
            IMPLMEMBER_REL(CHealth*, Health, 0, Endurance);
            IMPLMEMBER_REL(CInventory*, Inventory, 0, Health);
        };

        class CContext {
            IMPLMEMBER(ANet::Array<CCharacter*>, CharacterArray, 0x24);
            IMPLMEMBER_REL(ANet::Array<CPlayer*>, PlayerArray, 0x4, CharacterArray);
            IMPLMEMBER_REL(CCharacter*, ControlledCharacter, 0x4, PlayerArray);
        };
    }

    struct CContext
    {
        IMPLMEMBER(CharClient::CContext*, CharContext, 0x38);
    };

    namespace AgentSelection
    {
        class CContext
        {
            IMPLMEMBER(Agent::CAgentBase*, AutoSelection, 0x1c);
            IMPLMEMBER(Agent::CAgentBase*, HoverSelection, 0x58);
            IMPLMEMBER(Agent::CAgentBase*, LockedSelection, 0x8c);
            IMPLMEMBER(D3DXVECTOR3, ScreenToWorld, 0xb8);
        };
    }

    namespace AgentView
    {
        class CAvAgent
        {
            IMPLVTFUNC(Agent::CAgentBase*, GetAgent, 0x30);
        };

        class CContext
        {
            IMPLMEMBER(ANet::Array<CAvAgent*>, AgentArray, 0x30);
        };
    }

    namespace WorldView
    {
        class CContext
        {
            IMPLVTFUNC(void, GetMetrics, 0x30, long, one, D3DXVECTOR3*, camPos, D3DXVECTOR3*, lookAt, float*, fov);

            IMPLMEMBER(int, CamStatus, 0x40); // 0: no world view, 1: in game world view, 2: loading screen

        public:
            bool IsCameraValid() {
                if (getCamStatus() == 1) return true;
                return false;
            }
            D3DXVECTOR3 GetCamPos() {
                if (!IsCameraValid()) return D3DXVECTOR3(0, 0, 0);
                D3DXVECTOR3 camPos, buf;
                float buf2;
                GetMetrics(1, &camPos, &buf, &buf2);
                return camPos;
            }
            D3DXVECTOR3 GetViewVec() {
                if (!IsCameraValid()) return D3DXVECTOR3(1, 0, 0);
                D3DXVECTOR3 camPos, lookAt, viewVec;
                float buf;
                GetMetrics(1, &camPos, &lookAt, &buf);
                D3DXVec3Normalize(&viewVec, &(lookAt-camPos));
                return viewVec;
            }
            float GetFov() {
                if (!IsCameraValid()) return 1.0f;
                D3DXVECTOR3 buf, buf2;
                float fov;
                GetMetrics(1, &buf, &buf2, &fov);
                return fov;
            }
        };
    }
}


namespace GameData
{
    struct CharacterData;

    struct AgentData
    {
        AgentData() : pos(0, 0, 0)
        {
            pAgent = nullptr;
            pCharData = nullptr;
            category = GW2::AgentCategory::AGENT_CATEGORY_CHAR;
            type = GW2::AgentType::AGENT_TYPE_CHAR;
            agentId = 0;
            rot = 0;
        }

        GW2::Agent::CAgentBase *pAgent;
        CharacterData *pCharData;
        GW2::AgentCategory category;
        GW2::AgentType type;
        int agentId;
        D3DXVECTOR3 pos;
        float rot;
    };

    struct CharacterData
    {
        CharacterData()
        {
            pCharacter = nullptr;
            pAgentData = nullptr;
            isAlive = false;
            isDowned = false;
            isControlled = false;
            isPlayer = false;
            isInWater = false;
            isMonster = false;
            isMonsterPlayerClone = false;
            level = 0;
            scaledLevel = 0;
            wvwsupply = 0;
            currentHealth = 0;
            maxHealth = 0;
            currentEndurance = 0;
            maxEndurance = 0;
            profession = GW2::Profession::PROFESSION_NONE;
            attitude = GW2::Attitude::ATTITUDE_FRIENDLY;
        }

        GW2::CharClient::CCharacter *pCharacter;
        AgentData *pAgentData;
        bool isAlive;
        bool isDowned;
        bool isControlled;
        bool isPlayer;
        bool isInWater;
        bool isMonster;
        bool isMonsterPlayerClone;
        int level;
        int scaledLevel;
        int wvwsupply;
        float currentHealth;
        float maxHealth;
        float currentEndurance;
        float maxEndurance;
        GW2::Profession profession;
        GW2::Attitude attitude;
        std::string name;
    };

    struct GameData
    {
        GameData() : mouseInWorld(0, 0, 0)
        {
            mapId = 0;
        }

        struct ObjectData
        {
            ObjectData()
            {
                ownCharacter = nullptr;
                ownAgent = nullptr;
                autoSelection = nullptr;
                hoverSelection = nullptr;
                lockedSelection = nullptr;
            }

            std::vector<std::unique_ptr<CharacterData>> charDataList;
            std::vector<std::unique_ptr<AgentData>> agentDataList;
            CharacterData *ownCharacter;
            AgentData *ownAgent;
            AgentData *autoSelection;
            AgentData *hoverSelection;
            AgentData *lockedSelection;
        } objData;

        struct CamData
        {
            CamData() : camPos(0, 0, 0), viewVec(0, 0, 0)
            {
                valid = false;
                fovy = 0;
            }

            bool valid;
            D3DXVECTOR3 camPos;
            D3DXVECTOR3 viewVec;
            float fovy;
        } camData;

        D3DXVECTOR3 mouseInWorld;
        int mapId;
    };

    CharacterData *GetCharData(GW2::CharClient::CCharacter *pChar);
    AgentData *GetAgentData(GW2::Agent::CAgentBase *pAgent);
}


namespace GW2LIB
{
    struct Mems
    {
        // CContext
        // CharClient::CContext* m_charContext;
        uintptr_t contextChar = 0x38;

        // AgentView::CContext
        // ANet::Array<Agent::CAvAgent*> m_agentArray;
        uintptr_t avctxAgentArray = 0x30;

        // AgentView::CAvAgent
        // Agent::CAgentBase* GetAgent();
        uintptr_t avagVtGetAgent = 0x30;

        // Agent::CAgentBase
        // AgentCategory GetCategory();
        uintptr_t agentVtGetCategory = 0x10;
        // int GetAgentId();
        uintptr_t agentVtGetId = 0x5c;
        // AgentType GetType();
        uintptr_t agentVtGetType = 0x9c;
        // void GetPos(D3DXVECTOR3* pPos);
        uintptr_t agentVtGetPos = 0xb4;
        // Agent::CAgentTransform* m_transform;
        uintptr_t agentTransform = 0x1c;

        // Agent::CAgentTransform
        // float x;
        uintptr_t agtransX = 0x20;
        // float y;
        uintptr_t agtransY = 0x24;
        // float z;
        uintptr_t agtransZ = 0x28;
        // float rx;
        uintptr_t agtransRX = 0x100;
        // float ry;
        uintptr_t agtransRY = 0x104;

        // CharClient::CContext
        // ANet::Array<CharClient::CCharacter*> m_charArray;
        uintptr_t charctxCharArray = 0x24;
        // ANet::Array<CharClient::CPlayer*> m_playerArray;
        uintptr_t charctxPlayerArray = 0x34;
        // CharClient::CCharacter* m_controlledCharacter;
        uintptr_t charctxControlled = 0x44;

        // CharClient::CCharacter
        // Agent::CAgentBase* GetAgent();
        uintptr_t charVtGetAgent = 0x90;
        // int GetAgentId();
        uintptr_t charVtGetAgentId = 0x94;
        // CharClient::CPlayer* GetPlayer();
        uintptr_t charVtGetPlayer = 0xd0;
        // bool IsAlive();
        uintptr_t charVtAlive = 0x10c;
        // bool IsControlled();
        uintptr_t charVtControlled = 0x110;
        // bool IsDowned();
        uintptr_t charVtDowned = 0x11c;
        // bool IsInWater();
        uintptr_t charVtInWater = 0x13c;
        // bool IsMonster();
        uintptr_t charVtMonster = 0x14c;
        // bool IsMonsterPlayerClone();
        uintptr_t charVtClone = 0x15c;
        // bool IsPlayer();
        uintptr_t charVtPlayer = 0x178;
        // Attitude m_attitudeTowardsControlled;
        uintptr_t charAttitude = 0x58;
        // CharClient::CCoreStats* m_coreStats;
        uintptr_t charCoreStats = 0x148;
        // CharClient::CEndurance* m_endurance;
        uintptr_t charEndurance = 0x168;
        // CharClient::CHealth* m_health;
        uintptr_t charHealth = 0x16c;
        // CharClient::CInventory* m_inventory;
        uintptr_t charInventory = 0x170;

        // CharClient::CPlayer
        // char* m_name
        uintptr_t playerName = 0x30;

        // CharClient::CCoreStats
        // int m_level;
        uintptr_t statsLevel = 0x184;
        // int m_scaledLevel;
        uintptr_t statsScaledLevel = 0x1ac;
        // Profession m_profession;
        uintptr_t statsProfession = 0x1ec;

        // CharClient::CEndurance
        // int m_currentValue;
        uintptr_t endCurrent = 0x8;
        // int m_maxValue;
        uintptr_t endMax = 0xc;

        // CharClient::CHealth
        // int m_currentValue;
        uintptr_t healthCurrent = 0x8;
        // int m_maxValue;
        uintptr_t healthMax = 0xc;

        // CharClient::CInventory
        // int m_supply
        uintptr_t invSupply = 0x1d8;

        // AgentSelection::CContext
        // Agent::CAgentBase* m_autoSelection;
        uintptr_t asctxAuto = 0x24;
        // Agent::CAgentBase* m_hoverSelection;
        uintptr_t asctxHover = 0x70;
        // Agent::CAgentBase* m_lockedSelection;
        uintptr_t asctxLocked = 0xe0;
        // D3DXVECTOR3 m_screenToWorld;
        uintptr_t asctxStoW = 0x140;

        // WorldView::CContext
        // void GetMetrics(int one, D3DXVECTOR3* camPos, D3DXVECTOR3* lookAt, float* fov);
        uintptr_t wvctxVtGetMetrics = 0x34;
        // int m_camStatus;
        uintptr_t wvctxStatus = 0x40;

    };
}


void RenderCallback();


static const int CIRCLE_RES = 64;
static const VertexBuffer *vbCircle;


class MyMain : hl::Main
{
public:
    bool init() override
    {
        m_bPublicDrawer = false;
        m_cbRender = RenderCallback;

        m_hkDevice = nullptr;
        m_hkAlertCtx = nullptr;

        hMutexGameThread = CreateMutex(NULL, FALSE, NULL);
        hMutexRenderThread = CreateMutex(NULL, FALSE, NULL);


        uintptr_t MapIdSig = FindPattern("\00\x00\x08\x00\x89\x0d", "xxxxxx");

        PatternScanner scanner;
        auto results = scanner.find({
            "ViewAdvanceDevice",
            "ViewAdvanceAgentSelect",
            "ViewAdvanceAgentView",
            "ViewAdvanceWorldView",
            "character->IsAlive() || (character->IsDowned() && character->IsInWater())"
        });

        uintptr_t pAlertCtx = 0;
        if (![&](){
            __try {
                m_mems.pAgentViewCtx = *(GW2::AgentView::CContext**)(FollowRelativeAddress(results[2] + 0xa) + 0x1);

                pAlertCtx = **(uintptr_t**)(FollowRelativeAddress(results[0] + 0xa) + 0x1);

                m_mems.pAgentSelectionCtx = *(GW2::AgentSelection::CContext**)(FollowRelativeAddress(results[1] + 0xa) + 0x1);

                m_mems.ppWorldViewContext = *(GW2::WorldView::CContext***)(FollowRelativeAddress(results[3] + 0xa) + 0x1);

                m_mems.pMapId = *(int**)(MapIdSig + 0x6);

            } __except (EXCEPTION_EXECUTE_HANDLER) {
                return false;
            }

            return true;
        }())
        {
            HL_LOG("[Core::Init] One or more patterns are invalid\n");
            return false;
        }

        HL_LOG("aa:     %08X\n", m_mems.pAgentViewCtx);
        HL_LOG("actx:   %08X\n", pAlertCtx);
        HL_LOG("asctx:  %08X\n", m_mems.pAgentSelectionCtx);
        HL_LOG("wv:     %08X\n", m_mems.ppWorldViewContext);
        HL_LOG("mpid:   %08X\n", m_mems.pMapId);

        // hook functions
#ifdef NOD3DHOOK
        HL_LOG("Compiled to NOT hook D3D!\n");
#else
        // get d3d device
        LPDIRECT3DDEVICE9 pDevice = D3DDeviceFetcher::GetDeviceStealth();
        if (!pDevice) {
            HL_LOG("[Core::Init] Device not found\n");
            return false;
        }
        m_hkDevice = m_hooker.hookVT((uintptr_t)pDevice, 17, (uintptr_t)hkPresent);
        if (!m_hkDevice) {
            HL_LOG("[Core::Init] Hooking render thread failed\n");
            return false;
        }
        m_hkDevice = m_hooker.hookVT((uintptr_t)pDevice, 16, (uintptr_t)hkReset);
        if (!m_hkDevice) {
            HL_LOG("[Core::Init] Hooking device reset failed\n");
            return false;
        }
#endif
        m_hkAlertCtx = m_hooker.hookVT(pAlertCtx, 0, (uintptr_t)hkGameThread);
        if (!m_hkAlertCtx) {
            HL_LOG("[Core::Init] Hooking game thread failed\n");
            return false;
        }

        HL_LOG("Init ESP data\n");

        bool result = InitEsp();
#ifndef NOD3DHOOK
        if (!result)
            return false;
#endif

        return true;
    }

    bool step() override
    {
        // wait for close
        if (GetAsyncKeyState(VK_HOME) < 0) return false;
        return true;
    }

    void shutdown() override
    {
        if (m_hkDevice) {
            WaitForSingleObject(hMutexRenderThread, INFINITE);
            m_hooker.unhookVT(m_hkDevice);
        }
        ReleaseMutex(hMutexRenderThread);
        CloseHandle(hMutexRenderThread);

        if (m_hkAlertCtx) {
            WaitForSingleObject(hMutexGameThread, INFINITE);
            m_hooker.unhookVT(m_hkAlertCtx);
        }
        ReleaseMutex(hMutexGameThread);
        CloseHandle(hMutexGameThread);
    }

    Drawer *GetDrawer(bool bUsedToRender)
    {
        if (m_drawer.GetDevice() && (!bUsedToRender || m_bPublicDrawer))
            return &m_drawer;
        return nullptr;
    }

    bool InitEsp()
    {
        int c = 0;
        auto pDrawer = GetDrawer(false);
        while (!pDrawer) {
            if (c++ > 100) {
                HL_LOG("[GW2LIB::EnableEsp] waiting for drawer timed out\n");
                return false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            pDrawer = GetDrawer(false);
        }

        std::vector<VERTEX_3D_COL> verts;
        verts.resize(CIRCLE_RES+1);
        for (size_t i = 0; i < verts.size(); i++) {
            verts[i].x = cos(D3DX_PI*((CIRCLE_RES-i)/(CIRCLE_RES/2.0f)));
            verts[i].y = sin(D3DX_PI*((CIRCLE_RES-i)/(CIRCLE_RES/2.0f)));
            verts[i].z = 0;
        }

        vbCircle = pDrawer->AllocVertexBuffer(verts);
        if (vbCircle)
            return true;

        HL_LOG("[InitEsp] could not init esp data\n");
        return false;
    }

    void RenderHook(LPDIRECT3DDEVICE9 pDevice)
    {
        if (!m_drawer.GetDevice())
            m_drawer.SetDevice(pDevice);

        if (m_gameData.camData.valid) {
            D3DXMATRIX viewMat, projMat;
            D3DVIEWPORT9 viewport;
            pDevice->GetViewport(&viewport);
            D3DXMatrixLookAtLH(&viewMat, &m_gameData.camData.camPos, &(m_gameData.camData.camPos+m_gameData.camData.viewVec), &D3DXVECTOR3(0, 0, -1));
            D3DXMatrixPerspectiveFovLH(&projMat, m_gameData.camData.fovy, static_cast<float>(viewport.Width)/viewport.Height, 0.01f, 100000.0f);
            m_drawer.Update(viewMat, projMat);

            if (GetAsyncKeyState(VK_NUMPAD1) < 0) {
                pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
            }
            pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
            pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
            pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
            pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);

            // draw rect to display active rendering
            m_drawer.DrawRectFilled(0, 0, 3, 3, 0x77ffff00);

            if (m_cbRender) {
                m_bPublicDrawer = true;

                [&]()
                {
                    __try {
                        m_cbRender();
                    } __except (EXCEPTION_EXECUTE_HANDLER) {
                        HL_LOG("[ESP callback] Exception in ESP code\n");
                    }
                }();

                m_bPublicDrawer = false;
            }
        }
    }

    void RefreshDataAgent(GameData::AgentData *pAgentData, GW2::Agent::CAgentBase *pAgent)
    {
        // read data
        __try {
            ForeignClass agent = (uintptr_t)pAgent;

            pAgentData->pAgent = pAgent;

            pAgentData->category = agent.call<GW2::AgentCategory>(m_pubmems.agentVtGetCategory);
            pAgentData->type = agent.call<GW2::AgentType>(m_pubmems.agentVtGetType);
            pAgentData->agentId = agent.call<int>(m_pubmems.agentVtGetId);

            agent.call<void>(m_pubmems.agentVtGetPos, &pAgentData->pos);
            ForeignClass transform = agent.get<uintptr_t>(m_pubmems.agentTransform);
            if (transform)
            {
                pAgentData->rot = atan2(transform.get<float>(m_pubmems.agtransRY), transform.get<float>(m_pubmems.agtransRX));
            }

        } __except (EXCEPTION_EXECUTE_HANDLER) {
            HL_LOG("[RefreshDataAgent] access violation\n");
        }
    }
    void RefreshDataCharacter(GameData::CharacterData *pCharData, GW2::CharClient::CCharacter *pCharacter)
    {
        // read data
        __try {
            ForeignClass character = (uintptr_t)pCharacter;

            pCharData->pCharacter = pCharacter;

            pCharData->isAlive = character.call<bool>(m_pubmems.charVtAlive);
            pCharData->isDowned = character.call<bool>(m_pubmems.charVtDowned);
            pCharData->isControlled = character.call<bool>(m_pubmems.charVtControlled);
            pCharData->isPlayer = character.call<bool>(m_pubmems.charVtPlayer);
            pCharData->isInWater = character.call<bool>(m_pubmems.charVtInWater);
            pCharData->isMonster = character.call<bool>(m_pubmems.charVtMonster);
            pCharData->isMonsterPlayerClone = character.call<bool>(m_pubmems.charVtClone);

            pCharData->attitude = character.get<GW2::Attitude>(m_pubmems.charAttitude);

            ForeignClass health = character.get<uintptr_t>(m_pubmems.charHealth);
            if (health) {
                pCharData->currentHealth = health.get<float>(m_pubmems.healthCurrent);
                pCharData->maxHealth = health.get<float>(m_pubmems.healthMax);
            }

            ForeignClass endurance = character.get<uintptr_t>(m_pubmems.charEndurance);
            if (endurance) {
                pCharData->currentEndurance = static_cast<float>(endurance.get<int>(m_pubmems.endCurrent));
                pCharData->maxEndurance = static_cast<float>(endurance.get<int>(m_pubmems.endMax));
            }

            ForeignClass corestats = character.get<uintptr_t>(m_pubmems.charCoreStats);
            if (corestats) {
                pCharData->profession = corestats.get<GW2::Profession>(m_pubmems.statsProfession);
                pCharData->level = corestats.get<int>(m_pubmems.statsLevel);
                pCharData->scaledLevel = corestats.get<int>(m_pubmems.statsScaledLevel);
            }

            ForeignClass inventory = character.get<uintptr_t>(m_pubmems.charInventory);
            if (inventory) {
                pCharData->wvwsupply = inventory.get<int>(m_pubmems.invSupply);
            }

            if (pCharData->isPlayer)
            {
                ForeignClass player = character.call<uintptr_t>(m_pubmems.charVtGetPlayer);
                if (player)
                {
                    char *name = player.get<char*>(m_pubmems.playerName);
                    int i = 0;
                    pCharData->name = "";
                    while (name[i]) {
                        pCharData->name += name[i];
                        i += 2;
                    }
                }
            }



            /*pCharData->pCharacter = pCharacter;

            pCharData->isAlive = pCharacter->IsAlive();
            pCharData->isDowned = pCharacter->IsDowned();
            pCharData->isControlled = pCharacter->IsControlled();
            pCharData->isPlayer = pCharacter->IsPlayer();
            pCharData->isInWater = pCharacter->IsInWater();
            pCharData->isMonster = pCharacter->IsMonster();
            pCharData->isMonsterPlayerClone = pCharacter->IsMonsterPlayerClone();

            pCharData->attitude = pCharacter->getAttitudeTowardControlled();

            if (pCharacter->getHealth()) {
            pCharData->currentHealth = pCharacter->getHealth()->getCurrentValue();
            pCharData->maxHealth = pCharacter->getHealth()->getMaxValue();
            }

            if (pCharacter->getEndurance()) {
            pCharData->currentEndurance = static_cast<float>(pCharacter->getEndurance()->getCurrentValue());
            pCharData->maxEndurance = static_cast<float>(pCharacter->getEndurance()->getMaxValue());
            }

            if (pCharacter->getCoreStats()) {
            pCharData->profession = pCharacter->getCoreStats()->getProf();
            pCharData->level = pCharacter->getCoreStats()->getLevel();
            pCharData->scaledLevel = pCharacter->getCoreStats()->getScaledLevel();
            }

            if (pCharacter->getInventory()) {
            pCharData->wvwsupply = pCharacter->getInventory()->getSupply();
            }

            if (pCharData->isPlayer && pCharacter->GetPlayer()) {
            char *name = pCharacter->GetPlayer()->getName();
            int i = 0;
            pCharData->name = "";
            while (name[i]) {
            pCharData->name += name[i];
            i += 2;
            }
            }*/
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            HL_LOG("[RefreshDataCharacter] access violation\n");
        }
    }

    void GameHook()
    {
        uintptr_t **pLocalStorage;
        __asm {
            MOV EAX, DWORD PTR FS : [0x2c];
            MOV pLocalStorage, EAX;
        }
        m_mems.pCtx = (GW2::CContext*)(pLocalStorage[0][1]);

        // get cam data
        m_gameData.camData.valid = false;
        if (m_mems.ppWorldViewContext)
        {
            ForeignClass wvctx = (uintptr_t)(*m_mems.ppWorldViewContext);
            if (wvctx && wvctx.get<int>(m_pubmems.wvctxStatus) == 1)
            {
                D3DXVECTOR3 lookAt, upVec;
                wvctx.call<void>(m_pubmems.wvctxVtGetMetrics, 1, &m_gameData.camData.camPos, &lookAt, &m_gameData.camData.fovy, &upVec);
                D3DXVec3Normalize(&m_gameData.camData.viewVec, &(lookAt-m_gameData.camData.camPos));
                m_gameData.camData.valid = true;
            }
        }

        bool bOwnCharFound = false;
        bool bOwnAgentFound = false;
        bool bAutoSelectionFound = false;
        bool bHoverSelectionFound = false;
        bool bLockedSelectionFound = false;

        ForeignClass avctx = (uintptr_t)m_mems.pAgentViewCtx;
        ForeignClass asctx = (uintptr_t)m_mems.pAgentSelectionCtx;

        if (m_gameData.camData.valid && m_mems.pCtx)
        {
            ForeignClass ctx = (uintptr_t)m_mems.pCtx;
            if (ctx)
            {
                ForeignClass charctx = ctx.get<uintptr_t>(m_pubmems.contextChar);
                if (charctx && avctx && asctx)
                {
                    auto charArray = charctx.get<GW2::ANet::Array<GW2::CharClient::CCharacter*>>(m_pubmems.charctxCharArray);
                    auto agentArray = avctx.get<GW2::ANet::Array<GW2::AgentView::CAvAgent*>>(m_pubmems.avctxAgentArray);

                    if (charArray.IsValid() && agentArray.IsValid())
                    {
                        // add agents from game array to own array and update data
                        size_t sizeAgentArray = agentArray.Count();
                        if (sizeAgentArray != m_gameData.objData.agentDataList.size()) {
                            m_gameData.objData.agentDataList.resize(sizeAgentArray);
                        }
                        for (size_t i = 0; i < sizeAgentArray; i++)
                        {
                            GW2::AgentView::CAvAgent *pAvAgent = agentArray[i];

                            if (pAvAgent) {
                                GW2::Agent::CAgentBase *pAgent = pAvAgent->GetAgent();

                                if (pAgent) {
                                    // check if agent is already in our array
                                    GameData::AgentData *pAgentData = nullptr;

                                    if (m_gameData.objData.agentDataList[i] && m_gameData.objData.agentDataList[i]->pAgent == pAgent) {
                                        // agent is already in our array. fix ptr
                                        pAgentData = m_gameData.objData.agentDataList[i].get();
                                    }

                                    if (!pAgentData) {
                                        // agent is not in our array. add and fix ptr
                                        m_gameData.objData.agentDataList[i] = std::make_unique<GameData::AgentData>();
                                        pAgentData = m_gameData.objData.agentDataList[i].get();
                                    }

                                    // update values
                                    RefreshDataAgent(pAgentData, pAgent);

                                    bool bCharDataFound = false;

                                    if (!bCharDataFound) {
                                        pAgentData->pCharData = nullptr;
                                    }

                                    // set own agent
                                    if (m_gameData.objData.ownCharacter && m_gameData.objData.ownCharacter->pAgentData == pAgentData) {
                                        m_gameData.objData.ownAgent = pAgentData;
                                        bOwnAgentFound = true;
                                    }

                                    // set selection agents
                                    if (pAgent == asctx.get<GW2::Agent::CAgentBase*>(m_pubmems.asctxAuto)) {
                                        m_gameData.objData.autoSelection = pAgentData;
                                        bAutoSelectionFound = true;
                                    }
                                    if (pAgent == asctx.get<GW2::Agent::CAgentBase*>(m_pubmems.asctxHover)) {
                                        m_gameData.objData.hoverSelection = pAgentData;
                                        bHoverSelectionFound = true;
                                    }
                                    if (pAgent == asctx.get<GW2::Agent::CAgentBase*>(m_pubmems.asctxLocked)) {
                                        m_gameData.objData.lockedSelection = pAgentData;
                                        bLockedSelectionFound = true;
                                    }
                                }
                            }
                        }

                        // remove non valid agents from list
                        for (size_t i = 0; i < m_gameData.objData.agentDataList.size(); i++) {
                            if (!m_gameData.objData.agentDataList[i]) {
                                continue;
                            }

                            // check if agent in our array is in game data
                            bool bFound = false;

                            if (i < sizeAgentArray && agentArray[i]) {
                                if (agentArray[i]->GetAgent() == m_gameData.objData.agentDataList[i]->pAgent) {
                                    // agent was found in game. everything is fine
                                    bFound = true;
                                }
                            }

                            if (!bFound) {
                                // agent was not found in game. remove from our array
                                m_gameData.objData.agentDataList[i] = nullptr;
                            }
                        }

                        // add characters from game array to own array and update data
                        m_gameData.objData.charDataList.clear();
                        int sizeCharArray = charArray.Count();
                        for (int i = 0; i < sizeCharArray; i++)
                        {
                            GW2::CharClient::CCharacter *pCharacter = charArray[i];

                            if (pCharacter) {
                                m_gameData.objData.charDataList.push_back(std::make_unique<GameData::CharacterData>());
                                GameData::CharacterData *pCharData = m_gameData.objData.charDataList.rbegin()->get();

                                // update values
                                RefreshDataCharacter(pCharData, pCharacter);

                                bool bAgentDataFound = false;

                                // link agentdata of corresponding agent
                                if (m_gameData.objData.agentDataList[pCharacter->GetAgentId()]) {
                                    pCharData->pAgentData = m_gameData.objData.agentDataList[pCharacter->GetAgentId()].get();
                                    pCharData->pAgentData->pCharData = pCharData;
                                    bAgentDataFound = true;
                                }

                                if (!bAgentDataFound) {
                                    pCharData->pAgentData = nullptr;
                                }

                                // set own character
                                if (pCharacter == charctx.get<GW2::CharClient::CCharacter*>(m_pubmems.charctxControlled)) {
                                    m_gameData.objData.ownCharacter = pCharData;
                                    bOwnCharFound = true;
                                }
                            }
                        }
                    }
                }
            }
        }

        if (!bOwnCharFound)
            m_gameData.objData.ownCharacter = nullptr;
        if (!bOwnAgentFound)
            m_gameData.objData.ownAgent = nullptr;
        if (!bAutoSelectionFound)
            m_gameData.objData.autoSelection = nullptr;
        if (!bHoverSelectionFound)
            m_gameData.objData.hoverSelection = nullptr;
        if (!bLockedSelectionFound)
            m_gameData.objData.lockedSelection = nullptr;

        m_gameData.mouseInWorld = asctx.get<D3DXVECTOR3>(m_pubmems.asctxStoW);

        m_gameData.mapId = *m_mems.pMapId;
    }

    const VTHook *m_hkDevice;
    const VTHook *m_hkAlertCtx;

    HANDLE hMutexGameThread;
    HANDLE hMutexRenderThread;
    std::mutex m_gameDataMutex;

private:
    Hooker m_hooker;
    Drawer m_drawer;

    GameData::GameData m_gameData;

    bool m_bPublicDrawer = false;
    void(*m_cbRender)();

    struct Mems {
        Mems()
        {
            pMapId = nullptr;
            pCtx = nullptr;
            pAgentViewCtx = nullptr;
            ppWorldViewContext = nullptr;
            pAgentSelectionCtx = nullptr;
        }

        int *pMapId;
        GW2::CContext *pCtx;
        GW2::AgentView::CContext *pAgentViewCtx;
        GW2::WorldView::CContext **ppWorldViewContext;
        GW2::AgentSelection::CContext *pAgentSelectionCtx;
    } m_mems;

    GW2LIB::Mems m_pubmems;

};


hl::StaticInit<MyMain> g_initObj;









void __fastcall hkGameThread(uintptr_t pInst, int, int arg)
{
    MyMain *pCore = g_initObj.getMain();
    WaitForSingleObject(pCore->hMutexGameThread, INFINITE);

    {
        std::lock_guard<std::mutex> lock(pCore->m_gameDataMutex);

        [&](){
            __try {
                pCore->GameHook();
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                HL_LOG("[hkGameThread] Exception in game thread\n");
            }
        }();
    }

    uintptr_t orgFunc = pCore->m_hkAlertCtx->getOrgFunc(0);
    ((void(__thiscall*)(uintptr_t, int))(orgFunc))(pInst, arg);

    ReleaseMutex(pCore->hMutexGameThread);
}
HRESULT __stdcall hkPresent(LPDIRECT3DDEVICE9 pDevice, RECT* pSourceRect, RECT* pDestRect, HWND hDestWindowOverride, RGNDATA* pDirtyRegion)
{
    MyMain *pCore = g_initObj.getMain();
    WaitForSingleObject(pCore->hMutexRenderThread, INFINITE);

    {
        std::lock_guard<std::mutex> lock(pCore->m_gameDataMutex);

        [&](){
            __try {
                pCore->RenderHook(pDevice);
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                HL_LOG("[hkPresent] Exception in render thread\n");
            }
        }();
    }

    uintptr_t orgFunc = pCore->m_hkDevice->getOrgFunc(17);
    HRESULT hr = ((HRESULT(__thiscall*)(IDirect3DDevice9*, IDirect3DDevice9*, RECT*, RECT*, HWND, RGNDATA*))(orgFunc))(pDevice, pDevice, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);

    ReleaseMutex(pCore->hMutexRenderThread);
    return hr;
}
HRESULT __stdcall hkReset(LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS *pPresentationParameters)
{
    MyMain *pCore = g_initObj.getMain();

    __try {
        pCore->GetDrawer(false)->OnLostDevice();
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        HL_LOG("[hkReset] Exeption in pre device reset hook\n");
    }

    uintptr_t orgFunc = pCore->m_hkDevice->getOrgFunc(16);
    HRESULT hr = ((HRESULT(__thiscall*)(IDirect3DDevice9*, IDirect3DDevice9*, D3DPRESENT_PARAMETERS*))(orgFunc))(pDevice, pDevice, pPresentationParameters);

    __try {
        pCore->GetDrawer(false)->OnResetDevice();
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        HL_LOG("[hkReset] Exception in post device reset hook\n");
    }

    return hr;
}





void RenderCallback()
{
    Drawer *drawer = g_initObj.getMain()->GetDrawer(true);

    drawer->DrawCircle(100, 100, 65, 0x77cc5599);
}