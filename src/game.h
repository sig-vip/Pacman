#pragma once

#include "DGLE.h"
#include <list>
#include <ctime>

#define OPENGL_LEGACY_BASE_OBJECTS // we will use legacy OpenGL
#include "DGLE_CoreRenderer.h"

using namespace DGLE;
using namespace std;

#define SCREEN_WIDTH	_game_resolution[0]
#define SCREEN_HEIGHT	_game_resolution[1]

#define ROUND(x) (((x) < 0) ? (int)((x) - 0.5) : (int)((x) + 0.5))

#define RANDOM ((double)rand() / RAND_MAX)

inline TColor4 ColorDarkGreen(uint8 alpha = 255) { return TColor4(0x00, 0x80, 0x00, alpha); }

const BYTE
	_cell_n = 31, _cell_m = 21,   //for standard & labyrinth mode
//  _cell_n = 41, _cell_m = 23,   //for random labyrinth mode
	_wall_width = 40, _h_offset = 35;
const WORD
	_hot_area_origin[2] = { _wall_width, _wall_width + _h_offset },
	_game_resolution[2] = { _cell_n * _wall_width, _cell_m * _wall_width + _h_offset };

inline void Detect_xy(BYTE i, BYTE j, WORD &x, WORD &y)
{
	x = i * _wall_width - _wall_width / 2 + _hot_area_origin[0];
	y = j * _wall_width - _wall_width / 2 + _hot_area_origin[1];
}

struct Cell
{
	BYTE i, j;   // object coordinates (in units of cell number)

	Cell(BYTE _i = 0, BYTE _j = 0) :
		i(_i), j(_j)
	{}
};

class GameObject : protected Cell
{
public:

	enum E_GAME_OBJECT_TYPE : char { GOT_SCORE, GOT_EXIT, GOT_BLACK_SCORE, GOT_BIG_SCORE, GOT_HELP_SCORE,
		GOT_PLAYER, GOT_BLUE_GHOST, GOT_RED_GHOST, GOT_PURPLE_GHOST, GOT_GREEN_GHOST, GOT_OLIVE_GHOST, GOT_AQUA_GHOST, GOT_TEAL_GHOST };

protected:

	class Game *pGame;

	IEngineCore *pEngineCore;
	IRender2D *pRender2D;

	const E_GAME_OBJECT_TYPE objType;

	const BYTE r;   // object radius
	WORD x, y;   // object coordinates on the screen
	TColor Color;

	void Detect_xy()
	{
		::Detect_xy(i, j, x, y);
	}

	void Detect_ij()
	{
		i = ceil((x - _hot_area_origin[0]) / (double)_wall_width);
		j = ceil((y - _hot_area_origin[1]) / (double)_wall_width);
	}

	void GenerateCoord(BYTE d_x, BYTE d_y);

private:

	void InitRender2D()
	{
		IRender *p_render;
		pEngineCore->GetSubSystem(ESS_RENDER, (IEngineSubSystem *&)p_render);
		p_render->SetClearColor(ColorWhite());
		p_render->GetRender2D(pRender2D);
	}

public:

	GameObject(Game * _pGame, IEngineCore *_pEngineCore, E_GAME_OBJECT_TYPE _objType, BYTE _r) :
		pGame(_pGame), pEngineCore(_pEngineCore), objType(_objType), r(_r)
	{
		InitRender2D();
	}

	GameObject(Game *_pGame, IEngineCore *_pEngineCore, E_GAME_OBJECT_TYPE _objType, TColor _Color, BYTE _r = 15) :
		pGame(_pGame), pEngineCore(_pEngineCore), objType(_objType), Color(_Color), r(_r)
	{
		InitRender2D();
	}

	virtual ~GameObject()
	{}

	char GetType() const
	{
		return objType;
	}

	void Set_xy(WORD _x, WORD _y)
	{
		x = _x; y = _y;
		Detect_ij();
	}

	void Get_xy(WORD &_x, WORD &_y) const
	{
		_x = x; _y = y;
	}

	void Get_ij(BYTE &_i, BYTE &_j) const
	{
		_i = i; _j = j;
	}

	bool CompareCell(const GameObject &obj) const
	{
		if ((i == obj.i) && (j == obj.j))
			return true;
		else
			return false;
	}

	bool CompareCoord(const GameObject &obj) const
	{
		int dx = x - obj.x, dy = y - obj.y,
			R = r + obj.r;

		if ((dx * dx + dy * dy) <= R * R)
			return true;
		else
			return false;
	}

	virtual void Draw() const = 0;
};

class Item : public GameObject
{
public:

	Item(Game *_pGame, IEngineCore *_pEngineCore, E_GAME_OBJECT_TYPE _objType = GOT_SCORE, BYTE _r = 5, BYTE d_x = 1, BYTE d_y = 1) :
		GameObject(_pGame, _pEngineCore, _objType, _r)
	{
		switch (_objType)
		{
			case GOT_SCORE:
				Color = ColorBlue();
				break;

			case GOT_EXIT:
				Color = ColorRed();
				break;

			case GOT_BLACK_SCORE:
				Color = ColorBlack();
				break;

			case GOT_BIG_SCORE:
				Color = ColorGreen();
				break;

			case GOT_HELP_SCORE:
				Color = ColorAqua();
		}
		GenerateCoord(d_x, d_y);
	}

	void Draw() const;
};

class Ghost : public GameObject
{
private:

	ICoreRenderer *pCoreRenderer;

	ISoundSample *pSndSwallowScore, *pSndSwallowBlackScore, *pSndSwallowBigScore, *pSndSwallowHelpScore,
		*pSndSwallowExit, *pSndSwallowGhost, *pSndLostScore, *pSndIncreaseLives;

	BYTE mi, mj,   // are used in order to determine the transition to the next cell
		Velocity;

	char WallFlag;   // stores information of a wall in the direction of motion

	bool Inc,   // controls an increase of the Angle
		DirX, DirInc;   // determine direction of motion

	double Angle,   // the opening angle of the mouth
		ProbAI;   // the probability of pursuit objectives

	void Update_mij()
	{
		mi = i;
		mj = j;
	}

	void SetProbAI(double _ProbAI)
	{
		ProbAI = _ProbAI;
	}

	inline char WallPresence() const;
	inline void UpdateCoord();
	inline void ChangeDirInc(bool _DirInc, char _WallFlag);
	void AI(const GameObject &obj);
	void AI(const GameObject &obj, bool call_type);

public:

	Ghost(Game *_pGame, IEngineCore *_pEngineCore, E_GAME_OBJECT_TYPE _objType = GOT_PLAYER, TColor _Color = ColorYellow(), BYTE d_x = 7, BYTE d_y = 5, bool _Inc = rand() % 2, double _Angle = RANDOM * M_PI / 4.) :
		GameObject(_pGame, _pEngineCore, _objType, _Color), WallFlag(0), Inc(_Inc), DirX(rand() % 2), DirInc(rand() % 2), Angle(_Angle)
	{
		pEngineCore->GetSubSystem(ESS_CORE_RENDERER, (IEngineSubSystem *&)pCoreRenderer);

		// We must be sure that we are working with legacy OpenGL implementation!
		E_CORE_RENDERER_TYPE type;
		pCoreRenderer->GetRendererType(type);
		if (type != CRT_OPENGL_LEGACY)
			pEngineCore->WriteToLogEx("This program will work only with Legacy OpenGL renderer!", LT_FATAL, __FILE__, __LINE__);

		IResourceManager *p_res_man;
		pEngineCore->GetSubSystem(ESS_RESOURCE_MANAGER, (IEngineSubSystem *&)p_res_man);

		p_res_man->GetResourceByName("SndSwallowScore", (IEngineBaseObject *&)pSndSwallowScore);
		p_res_man->GetResourceByName("SndSwallowBlackScore", (IEngineBaseObject *&)pSndSwallowBlackScore);
		p_res_man->GetResourceByName("SndSwallowBigScore", (IEngineBaseObject *&)pSndSwallowBigScore);
		p_res_man->GetResourceByName("SndSwallowHelpScore", (IEngineBaseObject *&)pSndSwallowHelpScore);
		p_res_man->GetResourceByName("SndSwallowExit", (IEngineBaseObject *&)pSndSwallowExit);
		p_res_man->GetResourceByName("SndSwallowGhost", (IEngineBaseObject *&)pSndSwallowGhost);
		p_res_man->GetResourceByName("SndLostScore", (IEngineBaseObject *&)pSndLostScore);
		p_res_man->GetResourceByName("SndIncreaseLives", (IEngineBaseObject *&)pSndIncreaseLives);

		GenerateCoord(d_x, d_y);
		Update_mij();
		UpdateParameters();
	}

	inline bool Pathfind(const GameObject &obj) const;
	void UpdateParameters();
	bool Swallow(const GameObject &obj) const;

	void SetDirection(bool _DirX, bool _DirInc)
	{
		DirX = _DirX;
		DirInc = _DirInc;
	}

	void UpdateAI(double call_prob, bool call_type);
	inline void UpdateAI(double call_prob);
	inline void UpdateAngle();
	void Update();
	void Draw() const;
};

class Game : private IEngineCallback
{
private:

	IEngineCore *pEngineCore;
	IInput *pInput;
	IRender2D *pRender2D;

	IBitmapFont *pFont;

	const char *Mode;

	bool Pause, PauseChange, Difficulty, GameSpeed,
		BlackFlag, HelpFlag, IncreaseLivesFlag;

	BYTE ScoreNumber, BlackScoreNumber;
	uint CurrentScore, PreviousScore, CurrentLevel, PreviousLevel, Lives, NewLifeScoreDemand;

	char cell[_cell_n][_cell_m];
	list<Ghost> ghost;
	list<Item> item;
	Item *pNearest;
	TColor WallColor;
	uint Counter;

	Game(const Game &src) = delete;

	Game &operator=(const Game &src) = delete;

	void InitCell();
	void RestartLevel(bool restart);
	void NewGame();

	DGLE_RESULT DGLE_API Initialize();
	DGLE_RESULT DGLE_API Free();
	DGLE_RESULT DGLE_API Update(uint uiDeltaTime);
	DGLE_RESULT DGLE_API Render();
	DGLE_RESULT DGLE_API OnEvent(E_EVENT_TYPE eEventType, IBaseEvent *pEvent);

public:

	Game(IEngineCore *_pEngineCore, const char *_Mode = "s") :
		pEngineCore(_pEngineCore), pInput(NULL), pFont(NULL),
		Mode(_Mode), Pause(false), PauseChange(true), Difficulty(true), GameSpeed(true),
		ScoreNumber(40), BlackScoreNumber(10),   //ScoreNumber should be multiple of 4
		NewLifeScoreDemand(ScoreNumber + ScoreNumber / 2 + 6 * ScoreNumber / 4 + 80),   //NewLifeScoreDemand should depend on max number of ghosts
		pNearest(NULL), WallColor(ColorDarkGreen()), Counter(0)
	{
		pEngineCore->AddEngineCallback(this);
		srand(time(NULL));
	}

	~Game()
	{
		delete pNearest;
	}

	uint Pathfind(BYTE i_start, BYTE j_start, BYTE i_finish, BYTE j_finish);

	const char *GetMode() const
	{
		return Mode;
	}

	bool IsDifficultyHard() const
	{
		return Difficulty;
	}

	bool IsGameSpeedFast() const
	{
		return GameSpeed;
	}

	void SetBlackFlag(bool _flag)
	{
		BlackFlag = _flag;
	}

	bool IsBlackFlagSet() const
	{
		return BlackFlag;
	}

	void SetHelpFlag(bool _flag)
	{
		HelpFlag = _flag;
	}

	bool IsHelpFlagSet() const
	{
		return HelpFlag;
	}

	void SetIncreaseLivesFlag(bool _flag)
	{
		IncreaseLivesFlag = _flag;
	}

	bool IsIncreaseLivesFlagSet() const
	{
		return IncreaseLivesFlag;
	}

	BYTE GetScoreNumber() const
	{
		return ScoreNumber;
	}

	uint &refCurrentScore()
	{
		return CurrentScore;
	}

	uint &refCurrentLevel()
	{
		return CurrentLevel;
	}

	uint GetNewLifeScoreDemand() const
	{
		return NewLifeScoreDemand;
	}

	char GetCell(BYTE i, BYTE j) const
	{
		return cell[i][j];
	}

	const list<Ghost> &cGhostList() const
	{
		return ghost;
	}

	list<Ghost> &GhostList()
	{
		return ghost;
	}

	const list<Item> &cItemList() const
	{
		return item;
	}

	list<Item> &ItemList()
	{
		return item;
	}

	Item *GetNearest() const
	{
		return pNearest;
	}

	IDGLE_BASE_IMPLEMENTATION(IEngineCallback, INTERFACE_IMPL_END)
};