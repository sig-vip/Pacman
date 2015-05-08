#include "game.h"

#pragma comment(linker, "/defaultlib:opengl32.lib")
#pragma comment(linker, "/defaultlib:glu32.lib")
#pragma message("Linking with \"opengl32.lib\" and \"glu32.lib\".")

// game main resource path
#ifdef _DEBUG
#	define RESOURCE_PATH "..\\resources\\"
#else
#	define RESOURCE_PATH "..\\resources\\"
#endif

// determines whether one can pass from cell[i_start][j_start] to cell[i_finish][j_finish] or not
// returns the length of path
uint Game::Pathfind(BYTE i_start, BYTE j_start, BYTE i_finish, BYTE j_finish)
{
#define CHECKFRONT \
	if (pf_cell[i][j]) \
	{ \
		front.emplace_back(i, j); \
		pf_cell[i][j] = 0; \
		if (i == i_finish && j == j_finish) \
			flag = true; \
	}

	bool flag = false;
	uint count = 0;
	char pf_cell[_cell_n][_cell_m];
	list<Cell> current, front;

	if (cell[i_start][j_start])
	{
		current.clear();
		front.clear();

		for (BYTE i = 0; i < _cell_n; ++i)
			for (BYTE j = 0; j < _cell_m; ++j)
				pf_cell[i][j] = cell[i][j];

		current.emplace_back(i_start, j_start);
		while (!current.empty() && !flag)
		{
			for (auto current_it = current.cbegin(); current_it != current.cend() && !flag; ++current_it)
			{
				BYTE
					i = current_it->i - 1,
					j = current_it->j;
				CHECKFRONT

				i = current_it->i + 1;
				CHECKFRONT

				i = current_it->i;
				j = current_it->j - 1;
				CHECKFRONT

				j = current_it->j + 1;
				CHECKFRONT

				pf_cell[current_it->i][current_it->j] = 0;
			}
			current.assign(front.cbegin(), front.cend());
			front.clear();
			count++;
		}
	}

	if (!flag) count = 0;
	return count;
}

inline bool Ghost::Pathfind(const GameObject &obj) const
{
	BYTE objI, objJ;
	obj.Get_ij(objI, objJ);
	return pGame->Pathfind(i, j, objI, objJ);
}

// d_x, d_y - min distance between player and generated object (in units of cell number)
void GameObject::GenerateCoord(BYTE d_x, BYTE d_y)
{
	if (objType == GOT_PLAYER)
	{
		if (d_x != 0 || d_y != 0)
		{
			i = 1;
			j = 1;
		}
		else
		{
			// excludes generating object inside the wall or at the crossroads
			do
			{
				i = ROUND((_cell_n - 3)*RANDOM) + 1;
				j = ROUND((_cell_m - 3)*RANDOM) + 1;
			} while (!pGame->GetCell(i, j) || pGame->GetCell(i, j) == 4 || pGame->GetCell(i, j) == 5);
		}
	}
	else
	{
		do
		{
			i = ROUND((_cell_n - 3)*RANDOM) + 1;
			j = ROUND((_cell_m - 3)*RANDOM) + 1;
		} while (!pGame->GetCell(i, j) || abs(i - pGame->cGhostList().cbegin()->i) < d_x || abs(j - pGame->cGhostList().cbegin()->j) < d_y ||
			pGame->GetCell(i, j) == 4 || pGame->GetCell(i, j) == 5);
	}

	Detect_xy();
}

// updates ProbAI and Velocity according to Difficulty and Game Speed
void Ghost::UpdateParameters()
{
	switch (objType)
	{
		case GOT_PLAYER:   // is used for other ghosts
			if (pGame->IsDifficultyHard())
				ProbAI = 0.97;
			else
				ProbAI = 0.7;
			break;

		case GOT_BLUE_GHOST:
			if (pGame->IsDifficultyHard())
			{
				if (pGame->GetMode() == "s")
					ProbAI = 1;                //ProbAI + 0.02 * RANDOM;
				else
					ProbAI = 0.9;
			}
			else
				ProbAI = pGame->cGhostList().cbegin()->ProbAI + 0.2*RANDOM;
			break;

		case GOT_RED_GHOST:
			if (pGame->IsDifficultyHard() && pGame->GetMode() == "rl")
				ProbAI = 0.8;  //0.9;
			else
				ProbAI = pGame->cGhostList().cbegin()->ProbAI - 0.2*RANDOM;
			break;

		case GOT_GREEN_GHOST:
			ProbAI = pGame->cGhostList().cbegin()->ProbAI - 0.2*RANDOM;
			break;

		case GOT_PURPLE_GHOST:
		case GOT_TEAL_GHOST:
			ProbAI = pGame->cGhostList().cbegin()->ProbAI - 0.07;
			break;

		case GOT_OLIVE_GHOST:
			ProbAI = 0.5;
			break;

		case GOT_AQUA_GHOST:
			ProbAI = pGame->cGhostList().cbegin()->ProbAI - 0.1;
	}

	switch (objType)
	{
		case GOT_PLAYER:
			if (pGame->IsGameSpeedFast())
				Velocity = 3;
			else
				Velocity = 2;
			break;

		case GOT_RED_GHOST:
		case GOT_OLIVE_GHOST:
			Velocity = pGame->cGhostList().cbegin()->Velocity;
			break;

		case GOT_BLUE_GHOST:
		case GOT_PURPLE_GHOST:
		case GOT_GREEN_GHOST:
		case GOT_AQUA_GHOST:
		case GOT_TEAL_GHOST:
			Velocity = pGame->cGhostList().cbegin()->Velocity - 1;
	}
}

// implements interaction between objects
bool Ghost::Swallow(const GameObject &obj) const
{
	bool flag = false;

	if (objType == GOT_PLAYER)
	{
		if (CompareCell(obj))
		{
			switch (obj.GetType())
			{
				case GOT_SCORE:
					pGame->refCurrentScore()++;

					if (!(pGame->refCurrentScore() % pGame->GetNewLifeScoreDemand()))
						pGame->SetIncreaseLivesFlag(true);

					pSndSwallowScore->Play();
					break;

				case GOT_EXIT:
					pGame->refCurrentLevel()++;

					if (pGame->IsIncreaseLivesFlagSet())
						pSndIncreaseLives->Play();
					else
						pSndSwallowExit->Play();

					break;

				case GOT_BLACK_SCORE:
					// allows blue ghost to swallow score items
					pGame->SetBlackFlag(true);
					pSndSwallowBlackScore->Play();
					break;

				case GOT_BIG_SCORE:
					for (int k = 0; k < pGame->GetScoreNumber() / 2; ++k)
					{
						pGame->refCurrentScore()++;

						if (!(pGame->refCurrentScore() % pGame->GetNewLifeScoreDemand()))
							pGame->SetIncreaseLivesFlag(true);
					}

					pSndSwallowBigScore->Play();
					break;

				case GOT_HELP_SCORE:
					// allows olive ghost to swallow other ghosts producing pGame->GetScoreNumber() / 4 points from each one
					pGame->SetHelpFlag(true);
					pSndSwallowHelpScore->Play();
			}
			flag = true;
		}
	}
	else
	{
		if (obj.GetType() == GOT_PLAYER)
		{
			if (CompareCoord(obj))
			{
				flag = true;
				pSndSwallowGhost->Play();
			}
		}
		else if (objType == GOT_BLUE_GHOST)
		{
			if (obj.GetType() == GOT_SCORE)
				if (CompareCell(obj))
				{
					flag = true;
					pSndLostScore->Play();
				}
		}
		else if (objType == GOT_OLIVE_GHOST)
		{
			switch (obj.GetType())
			{
				case GOT_BLUE_GHOST:
				case GOT_RED_GHOST:
				case GOT_PURPLE_GHOST:
				case GOT_GREEN_GHOST:
				case GOT_AQUA_GHOST:
				case GOT_TEAL_GHOST:
					if (CompareCoord(obj))
					{
						flag = true;

						for (int k = 0; k < pGame->GetScoreNumber() / 4; ++k)
						{
							pGame->refCurrentScore()++;
							if (!(pGame->refCurrentScore() % pGame->GetNewLifeScoreDemand()))
								pGame->SetIncreaseLivesFlag(true);
						}

						pSndSwallowGhost->Play();
					}
			}
		}
	}

	return flag;
}

// determines the presence of wall around the object
inline char Ghost::WallPresence() const
{
	char flag = 'e';
	bool
		lx = false, rx = false,
		ty = false, by = false,
		lxty = false, lxby = false,
		rxty = false, rxby = false;

	WORD x = this->x - _hot_area_origin[0];
	WORD y = this->y - _hot_area_origin[1];
	BYTE velocity = Velocity + 1;

	if (!pGame->GetCell(i - 1, j))
		if ((x - r) <= (i - 1)*_wall_width + velocity)
			lx = true;

	if (!pGame->GetCell(i + 1, j))
		if ((x + r) >= i*_wall_width - velocity)
			rx = true;

	if (!pGame->GetCell(i, j - 1))
		if ((y - r) <= (j - 1)*_wall_width + velocity)
			ty = true;

	if (!pGame->GetCell(i, j + 1))
		if ((y + r) >= j*_wall_width - velocity)
			by = true;

	if (!pGame->GetCell(i - 1, j - 1))
		if (((y - r) <= (j - 1)*_wall_width) && ((x - r) <= (i - 1)*_wall_width))
			lxty = true;

	if (!pGame->GetCell(i - 1, j + 1))
		if (((y + r) >= j*_wall_width) && ((x - r) <= (i - 1)*_wall_width))
			lxby = true;

	if (!pGame->GetCell(i + 1, j - 1))
		if (((y - r) <= (j - 1)*_wall_width) && ((x + r) >= i*_wall_width))
			rxty = true;

	if (!pGame->GetCell(i + 1, j + 1))
		if (((y + r) >= j*_wall_width) && ((x + r) >= i*_wall_width))
			rxby = true;

	if (((lx || rx) && !(ty || by)) || ((ty || by) && !(lx || rx)))
	{
		if (lx)
		{
			if (rx)
				flag = 'x';
			else
				flag = 'l';
		}
		else if (rx)
			flag = 'r';
		else if (ty)
		{
			if (by)
				flag = 'y';
			else
				flag = 't';
		}
		else if (by)
			flag = 'b';
	}
	else if (lx && ty)
		flag = '1';
	else if (lx && by)
		flag = '2';
	else if (rx && by)
		flag = '3';
	else if (rx && ty)
		flag = '4';
	else if (!lx && !rx && !ty && !by)
	{
		if (lxty)
			flag = '5';
		else if (lxby)
			flag = '6';
		else if (rxty)
			flag = '7';
		else if (rxby)
			flag = '8';
		else
			flag = '0';
	}

	return flag;
}

// updates coordinates of the object
inline void Ghost::UpdateCoord()
{
	char wall = WallPresence();

	WallFlag = 0;

	if (DirX)
	{
		if (!DirInc)
		{
			if (wall != 'l' && wall != 'x' && wall != '1' && wall != '2')
			{
				x -= Velocity;
				if (wall == '5')
					y += Velocity;
				else if (wall == '6')
					y -= Velocity;
			}
			else
				WallFlag = 'l';
		}
		else
		{
			if (wall != 'r' && wall != 'x' && wall != '3' && wall != '4')
			{
				x += Velocity;
				if (wall == '7')
					y += Velocity;
				else if (wall == '8')
					y -= Velocity;
			}
			else
				WallFlag = 'r';
		}
	}
	else
	{
		if (!DirInc)
		{
			if (wall != 't' && wall != 'y' && wall != '1' && wall != '4')
			{
				y -= Velocity;
				if (wall == '5')
					x += Velocity;
				else if (wall == '7')
					x -= Velocity;
			}
			else
				WallFlag = 't';
		}
		else
		{
			if (wall != 'b' && wall != 'y' && wall != '2' && wall != '3')
			{
				y += Velocity;
				if (wall == '6')
					x += Velocity;
				else if (wall == '8')
					x -= Velocity;
			}
			else
				WallFlag = 'b';
		}
	}

	Detect_ij();
}

inline void Ghost::ChangeDirInc(bool _DirInc, char _WallFlag)
{
	if (WallFlag != _WallFlag)
	{
		DirInc = _DirInc;
		WallFlag = 0;
	}
}

// artificial intelligence
void Ghost::AI(const GameObject &obj)
{
	double p_x = 0.5;
	WORD objX, objY;
	obj.Get_xy(objX, objY);

	if (pGame->GetMode() == "s")
	{
		if (abs(x - objX) > abs(y - objY))
			p_x += 0.2 * RANDOM;
		else
			p_x -= 0.2 * RANDOM;
	}
	else if (pGame->IsDifficultyHard())
	{
		BYTE objI, objJ;
		obj.Get_ij(objI, objJ);
		uint count[4] =
		{
			pGame->Pathfind(i - 1, j, objI, objJ),
			pGame->Pathfind(i + 1, j, objI, objJ),
			pGame->Pathfind(i, j - 1, objI, objJ),
			pGame->Pathfind(i, j + 1, objI, objJ)
		};
		BYTE m = 0;
		for (BYTE n = 1; n < 4 && !count[m]; ++n)
			m = n;

		if (count[m])
		{
			for (BYTE n = 1; n < 4; ++n)
				if (count[m] > count[n] && count[n])
					m = n;

			switch (m)
			{
				case 0:
					::Detect_xy(i - 1, j, objX, objY);
					p_x += 0.05 * RANDOM;
					break;

				case 1:
					::Detect_xy(i + 1, j, objX, objY);
					p_x += 0.05 * RANDOM;
					break;

				case 2:
					::Detect_xy(i, j - 1, objX, objY);
					p_x -= 0.05 * RANDOM;
					break;

				case 3:
					::Detect_xy(i, j + 1, objX, objY);
					p_x -= 0.05 * RANDOM;
			}
		}
	}

	if (RANDOM <= p_x)
	{
		DirX = true;
		if (RANDOM <= ProbAI)
		{
			if (x < objX)
				ChangeDirInc(true, 'r');
			else
				ChangeDirInc(false, 'l');
		}
		else
		{
			if (x < objX)
				ChangeDirInc(false, 'l');
			else
				ChangeDirInc(true, 'r');
		}
	}
	else
	{
		DirX = false;
		if (RANDOM <= ProbAI)
		{
			if (y < objY)
				ChangeDirInc(true, 'b');
			else
				ChangeDirInc(false, 't');
		}
		else
		{
			if (y < objY)
				ChangeDirInc(false, 't');
			else
				ChangeDirInc(true, 'b');
		}
	}
}

// if (call_type) force a change in direction of the object
void Ghost::AI(const GameObject &obj, bool call_type)
{
	if (call_type)
		while (WallFlag)
			AI(obj);
	else
		AI(obj);
}

void Ghost::UpdateAI(double call_prob, bool call_type)
{
	if (RANDOM <= call_prob)
	{
		switch (objType)
		{
			case GOT_BLUE_GHOST:
				if (pGame->IsBlackFlagSet())
				{
					// set the nearest object as an objective
					WORD nearest_x, nearest_y;
					pGame->cGhostList().cbegin()->Get_xy(nearest_x, nearest_y);
					for (auto it = ++pGame->cItemList().cbegin(); it != pGame->cItemList().cend(); ++it)
					{
						if (it->GetType() == GOT_SCORE)
						{
							WORD it_x, it_y;
							it->Get_xy(it_x, it_y);
							if ((x - it_x)*(x - it_x) + (y - it_y)*(y - it_y) <
								(x - nearest_x)*(x - nearest_x) + (y - nearest_y)*(y - nearest_y))
							{
								nearest_x = it_x;
								nearest_y = it_y;
							}
						}
					}
					pGame->GetNearest()->Set_xy(nearest_x, nearest_y);
					AI(*pGame->GetNearest(), call_type);
				}
				else
					AI(*pGame->cGhostList().cbegin(), call_type);
				break;

			case GOT_RED_GHOST:
			case GOT_OLIVE_GHOST:
				AI(*pGame->cGhostList().cbegin(), call_type);
				break;

			case GOT_PURPLE_GHOST:
				AI(*pGame->cItemList().cbegin(), call_type);
				break;

			default:
				bool flag = false;
				for (auto it = ++pGame->cItemList().cbegin(); it != pGame->cItemList().cend(); ++it)
				{
					switch (objType)
					{
						case GOT_GREEN_GHOST:
							if (it->GetType() == GOT_BIG_SCORE)
							{
								AI(*it, call_type);
								flag = true;
							}
							break;

						case GOT_AQUA_GHOST:
							if (it->GetType() == GOT_HELP_SCORE)
							{
								AI(*it, call_type);
								flag = true;
							}
							break;

						case GOT_TEAL_GHOST:
							if (it->GetType() == GOT_HELP_SCORE)
							{
								for (auto i = ++pGame->cItemList().cbegin(); i != pGame->cItemList().cend(); ++i)
								{
									if (i->GetType() == GOT_BIG_SCORE)
									{
										AI(*it, call_type);
										flag = true;
									}
								}
							}
					}
				}
				if (!flag)
					AI(*pGame->cGhostList().cbegin(), call_type);
		}
	}
}

// for easy difficulty level
inline void Ghost::UpdateAI(double call_prob)
{
	double _ProbAI = 1 - ProbAI;

	SetProbAI(_ProbAI);
	UpdateAI(call_prob, false);
	SetProbAI(1 - _ProbAI);
}

inline void Ghost::UpdateAngle()
{
	double v_angle = 0.01;

	switch (objType)
	{
		case GOT_PLAYER:
		case GOT_RED_GHOST:
		case GOT_OLIVE_GHOST:
			v_angle *= 2;
	}

	if (Angle >= M_PI / 4.)
		Inc = false;
	else if (Angle <= v_angle)
		Inc = true;

	if (Inc)
		Angle += v_angle;
	else
		Angle -= v_angle;
}

void Ghost::Update()
{
	UpdateCoord();

	if (objType != GOT_PLAYER)
	{
		UpdateAngle();
		UpdateAI(1, true);   // in order to ghosts don't stop

		// updating AI after moving to the next cell
		if (!(i == mi && j == mj))
		{
			if (pGame->IsDifficultyHard() && pGame->GetMode() == "rl")
			{
				if (Velocity == pGame->cGhostList().cbegin()->Velocity)
					UpdateAI(0.4, false);   //0.3
				else
					UpdateAI(0.8, false);   //0.2
			}
			else if (pGame->GetCell(i, j) == 4 || pGame->GetCell(i, j) == 5)
			{
				if (pGame->GetMode() == "s")
					UpdateAI(0.8, false);
				else
				{
					double call_prob = 0.8;             //0.5   0.8
					if (pGame->GetMode() == "rl")
						call_prob = 0.3;                //0.4   0.8   0.2

					if (pGame->IsDifficultyHard())
						// for labyrinth mode
						UpdateAI(call_prob, false);
					else
					{
						if (RANDOM < 0.7)
							UpdateAI(call_prob, false);
						else
							UpdateAI(call_prob);
					}
				}
			}
		}
	}

	Update_mij();   // isn't used if (objType == GOT_PLAYER) -> for future needs
}

void Item::Draw() const
{
	switch (objType)
	{
		case GOT_SCORE:
		case GOT_EXIT:
			pRender2D->DrawCircle(TPoint2(x, y), r, 32, Color, PF_FILL);
			break;

		case GOT_BLACK_SCORE:
			pRender2D->DrawCircle(TPoint2(x, y), r, 6, Color, PF_FILL);
			break;

		case GOT_BIG_SCORE:
		case GOT_HELP_SCORE:
			pRender2D->DrawCircle(TPoint2(x, y), r, 4, Color, PF_FILL);
	}
}

void Ghost::Draw() const
{
	int sign[4];
	double a = Angle;
	const double R = r / cos(a);

	if (DirX)
	{
		if (DirInc)
		{
			sign[0] = 1; sign[1] = -1;
			sign[2] = 1; sign[3] = 1;
		}
		else
		{
			sign[0] = -1; sign[1] = 1;
			sign[2] = -1; sign[3] = -1;
		}
	}
	else
	{
		a = M_PI / 2. - a;
		if (DirInc)
		{
			sign[0] = 1; sign[1] = 1;
			sign[2] = -1; sign[3] = 1;
		}
		else
		{
			sign[0] = -1; sign[1] = -1;
			sign[2] = 1; sign[3] = -1;
		}
	}

	const float cos_a = R * cos(a),
		sin_a = R * sin(a);

	glEnable(GL_STENCIL_TEST);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	pCoreRenderer->Clear(false, false, true);
	glStencilFunc(GL_ALWAYS, 1, ~0);
	glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);

	const TVertex2 triangle[] = { TVertex2(x, y, 0.f, 0.f, 1.f, 1.f, 1.f, 1.f),
		TVertex2(x + sign[0] * cos_a, y + sign[1] * sin_a, 0.f, 0.f, 1.f, 1.f, 1.f, 1.f),
		TVertex2(x + sign[2] * cos_a, y + sign[3] * sin_a, 0.f, 0.f, 1.f, 1.f, 1.f, 1.f) };

	pRender2D->DrawTriangles(NULL, triangle, 3, PF_FILL);

	glStencilFunc(GL_EQUAL, 0, ~0);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	pRender2D->DrawCircle(TPoint2(x, y), r, 32, Color, PF_FILL);
	glDisable(GL_STENCIL_TEST);
}

void Game::InitCell()
{
	BYTE i, j;

	// fill the area by presence of the wall
	for (i = 0; i < _cell_n; ++i)
		for (j = 0; j < _cell_m; ++j)
			cell[i][j] = 0;

	// remove the wall in some places
	double p = 0.6;
	if (GetMode() == "rl")
	{
		cell[1][1] = 1;
		for (i = 1; i < (_cell_n - 1); ++i)
			for (j = 1; j < (_cell_m - 1); ++j)
				if (RANDOM < p)
					cell[i][j] = 1;
	}
	else
	{
		for (i = 1; i < (_cell_n - 1); ++i)
		{
			cell[i][1] = 1;
			cell[i][3] = 1;
			cell[i][6] = 1;
			cell[i][8] = 1;
			cell[i][10] = 1;
			cell[i][12] = 1;
			cell[i][15] = 1;
			cell[i][17] = 1;
			cell[i][19] = 1;
		}
		for (j = 1; j < (_cell_m - 1); ++j)
		{
			cell[1][j] = 1;
			cell[4][j] = 1;
			cell[7][j] = 1;
			cell[11][j] = 1;
			cell[16][j] = 1;
			cell[18][j] = 1;
			cell[21][j] = 1;
			cell[23][j] = 1;
			cell[27][j] = 1;
		}
	}

	// determine crossroads
	for (i = 1; i < (_cell_n - 1); ++i)
	{
		for (j = 1; j < (_cell_m - 1); ++j)
			if (cell[i][j] > 0)
			{
				if (cell[i - 1][j] > 0) cell[i][j]++;
				if (cell[i][j - 1] > 0) cell[i][j]++;
				if (cell[i + 1][j] > 0) cell[i][j]++;
				if (cell[i][j + 1] > 0) cell[i][j]++;
			}
	}

	// add the wall for labyrinth mode
	p = 0.4;
	if (GetMode() == "l")
		for (i = 1; i < (_cell_n - 1); ++i)
			for (j = 1; j < (_cell_m - 1); ++j)
				if (cell[i][j] == 4 || cell[i][j] == 5)
					if (RANDOM < p)
						cell[i][j] = 0;
}

void Game::RestartLevel(bool restart)
{
	bool done = false;

	while (!done)
	{
		InitCell();

		GhostList().clear();
		ItemList().clear();

		// add player
		if (CurrentLevel < 8)
			GhostList().emplace_back(this, pEngineCore);
		else
			GhostList().emplace_back(this, pEngineCore, GameObject::GOT_PLAYER, ColorYellow(), 0, 0);

		// add AUX object
		if (!pNearest) pNearest = new Item(this, pEngineCore);

		// add exit from level
		ItemList().emplace_back(this, pEngineCore, GameObject::GOT_EXIT, 7, 7, 5);

		// add score items
		for (int i = 0; i < ScoreNumber; ++i)
			ItemList().emplace_back(this, pEngineCore);

		// add ghosts and special items
		GhostList().emplace_back(this, pEngineCore, GameObject::GOT_BLUE_GHOST, ColorBlue());
		if (CurrentLevel > 1)
			GhostList().emplace_back(this, pEngineCore, GameObject::GOT_RED_GHOST, ColorRed());
		if (CurrentLevel > 2)
		{
			for (int i = 0; i < BlackScoreNumber; ++i)
				ItemList().emplace_back(this, pEngineCore, GameObject::GOT_BLACK_SCORE, 7);

			SetBlackFlag(false);
		}
		if (CurrentLevel > 3)
			GhostList().emplace_back(this, pEngineCore, GameObject::GOT_PURPLE_GHOST, ColorPurple());
		if (CurrentLevel > 4)
		{
			ItemList().emplace_back(this, pEngineCore, GameObject::GOT_BIG_SCORE, 8, 5, 7);
			GhostList().emplace_back(this, pEngineCore, GameObject::GOT_GREEN_GHOST, ColorGreen(), 5, 7);
		}
		if (CurrentLevel > 5)
			GhostList().emplace_back(this, pEngineCore, GameObject::GOT_OLIVE_GHOST, ColorOlive(), 5, 5);
		if (CurrentLevel > 6)
		{
			ItemList().emplace_back(this, pEngineCore, GameObject::GOT_HELP_SCORE, 8, 7, 5);
			SetHelpFlag(false);
			GhostList().emplace_back(this, pEngineCore, GameObject::GOT_AQUA_GHOST, ColorAqua());
			GhostList().emplace_back(this, pEngineCore, GameObject::GOT_TEAL_GHOST, ColorTeal());
		}

		// if player cannot interact with some objects -> restart initialization
		done = true;
		if (GetMode() != "s")
		{
			for (auto it = cItemList().cbegin(); it != cItemList().cend() && done; ++it)
				if (!cGhostList().cbegin()->Pathfind(*it))
					done = false;

			// if (!done) the following loop is unnecessary and will not execute
			// some ghosts can be walled up if manually disable the following loop (it can be usefull for medium difficulty level)

			//for (auto it = ++cGhostList().cbegin(); it != cGhostList().cend() && done; ++it)
			//	if (!cGhostList().cbegin()->Pathfind(*it))
			//		done = false;
		}
	}

	if (restart)
		CurrentScore = PreviousScore;
	else
	{
		PreviousScore = CurrentScore;
		if (IncreaseLivesFlag)
			Lives++;
	}
	SetIncreaseLivesFlag(false);
}

void Game::NewGame()
{
	PreviousScore = 0;
	CurrentLevel = 1;
	PreviousLevel = CurrentLevel;
	Lives = 3;

	SetBlackFlag(false);
	SetHelpFlag(false);
	SetIncreaseLivesFlag(false);

	RestartLevel(true);
}

DGLE_RESULT DGLE_API Game::Initialize()
{
	pEngineCore->GetSubSystem(ESS_INPUT, (IEngineSubSystem *&)pInput);

	IRender *p_render;
	pEngineCore->GetSubSystem(ESS_RENDER, (IEngineSubSystem *&)p_render);
	p_render->SetClearColor(ColorWhite());
	p_render->GetRender2D(pRender2D);

	IResourceManager *p_res_man;
	pEngineCore->GetSubSystem(ESS_RESOURCE_MANAGER, (IEngineSubSystem *&)p_res_man);

	IEngineBaseObject *p_tmp_obj;

	p_res_man->GetDefaultResource(EOT_BITMAP_FONT, (IEngineBaseObject *&)pFont); // getting default font
	//p_res_man->Load(RESOURCE_PATH"\\fonts\\Times_New_Roman_12_rus.dft", (IEngineBaseObject *&)pFont, RES_LOAD_DEFAULT);

	p_res_man->Load(RESOURCE_PATH"\\sounds\\SwallowScore.wav", p_tmp_obj, RES_LOAD_DEFAULT, "SndSwallowScore");
	p_res_man->Load(RESOURCE_PATH"\\sounds\\SwallowBlackScore.wav", p_tmp_obj, RES_LOAD_DEFAULT, "SndSwallowBlackScore");
	p_res_man->Load(RESOURCE_PATH"\\sounds\\SwallowBigScore.wav", p_tmp_obj, RES_LOAD_DEFAULT, "SndSwallowBigScore");
	p_res_man->Load(RESOURCE_PATH"\\sounds\\SwallowHelpScore.wav", p_tmp_obj, RES_LOAD_DEFAULT, "SndSwallowHelpScore");
	p_res_man->Load(RESOURCE_PATH"\\sounds\\SwallowExit.wav", p_tmp_obj, RES_LOAD_DEFAULT, "SndSwallowExit");
	p_res_man->Load(RESOURCE_PATH"\\sounds\\SwallowGhost.wav", p_tmp_obj, RES_LOAD_DEFAULT, "SndSwallowGhost");
	p_res_man->Load(RESOURCE_PATH"\\sounds\\LostScore.wav", p_tmp_obj, RES_LOAD_DEFAULT, "SndLostScore");
	p_res_man->Load(RESOURCE_PATH"\\sounds\\IncreaseLives.wav", p_tmp_obj, RES_LOAD_DEFAULT, "SndIncreaseLives");

	NewGame();

	return S_OK;
}

DGLE_RESULT DGLE_API Game::Free()
{
	return S_OK;
}

DGLE_RESULT DGLE_API Game::Update(uint uiDeltaTime)
{
	bool key_escape, pause;

	pInput->GetKey(KEY_ESCAPE, key_escape);

	if (key_escape)
		pEngineCore->QuitEngine();

	pInput->GetKey(KEY_P, pause);

	if (pause)
	{
		if (PauseChange)
		{
			Pause = !Pause;
			PauseChange = false;
		}
	}
	else
		PauseChange = true;

	if (!Pause)
	{
		bool key_right, key_left, key_down, key_up;

		pInput->GetKey(KEY_RIGHT, key_right);
		pInput->GetKey(KEY_LEFT, key_left);
		pInput->GetKey(KEY_DOWN, key_down);
		pInput->GetKey(KEY_UP, key_up);

		// update game objects
		GhostList().begin()->UpdateAngle();
		if (key_right)
		{
			GhostList().begin()->SetDirection(true, true);
			GhostList().begin()->Update();
		}
		else if (key_left)
		{
			GhostList().begin()->SetDirection(true, false);
			GhostList().begin()->Update();
		}
		else if (key_down)
		{
			GhostList().begin()->SetDirection(false, true);
			GhostList().begin()->Update();
		}
		else if (key_up)
		{
			GhostList().begin()->SetDirection(false, false);
			GhostList().begin()->Update();
		}

		for (auto it = ++GhostList().begin(); it != GhostList().end(); ++it)
			it->Update();

		// swallow score and special items
		for (auto it = cItemList().cbegin(); it != cItemList().cend();)
		{
			bool flag = false;
			if (IsBlackFlagSet())
				for (auto this_ghost = ++cGhostList().cbegin(); this_ghost != cGhostList().cend(); ++this_ghost)
					if (this_ghost->Swallow(*it))
						flag = true;

			if (cGhostList().cbegin()->Swallow(*it) || flag)
				it = ItemList().erase(it);
			else
				++it;
		}
		if (CurrentLevel > PreviousLevel)
		{
			RestartLevel(false);
			PreviousLevel = CurrentLevel;
		}

		// swallow player and ghosts
		for (auto this_ghost = ++cGhostList().cbegin(); this_ghost != cGhostList().cend(); ++this_ghost)
		{
			if (this_ghost->Swallow(*cGhostList().cbegin()))
			{
				if (Lives > 1)
				{
					RestartLevel(true);
					Lives--;
				}
				else
					NewGame();

				break;
			}
			else if (IsHelpFlagSet())
				for (auto it = ++cGhostList().cbegin(); it != cGhostList().cend();)
					if (this_ghost->Swallow(*it))
						it = GhostList().erase(it);
					else
						++it;
		}

		// additional updating AI
		if (!(Counter % 15))
			for (auto it = ++GhostList().begin(); it != GhostList().end(); ++it)
				it->UpdateAI(0.1, false);     //0.25

		++Counter;
	}

	return S_OK;
}

DGLE_RESULT DGLE_API Game::Render()
{
	//pRender2D->Begin2D();

	// render game objects
	for (auto it = ++cItemList().cbegin(); it != cItemList().cend(); ++it)
		if (it->GetType() == GameObject::GOT_SCORE)
			it->Draw();

	for (auto it = ++cItemList().cbegin(); it != cItemList().cend(); ++it)
		if (it->GetType() == GameObject::GOT_BLACK_SCORE)
			it->Draw();

	for (auto it = ++cItemList().cbegin(); it != cItemList().cend(); ++it)
	{
		switch (it->GetType())
		{
			case GameObject::GOT_BIG_SCORE:
			case GameObject::GOT_HELP_SCORE:
				it->Draw();
		}
	}
	cItemList().cbegin()->Draw();
	for (auto const &ghost : cGhostList())
		ghost.Draw();

	// render wall
	for (BYTE i = 0; i < _cell_n; ++i)
		for (BYTE j = 0; j < _cell_m; ++j)
			if (!cell[i][j])
				pRender2D->DrawRectangle(TRectF(i * _wall_width, j * _wall_width + _h_offset, _wall_width, _wall_width), WallColor, PF_FILL);

	// render HUD
	char chLevel[6 + 16], chLives[7 + 16], chStatus[10], chScore[8 + 16];

	sprintf_s(chLevel, "Level %u", CurrentLevel);
	sprintf_s(chLives, "Lives: %u", Lives);
	if (CurrentLevel == 1)
		sprintf_s(chStatus, "New Game!");
	else
		sprintf_s(chStatus, "Hurry up!");
	sprintf_s(chScore, "Score = %u", CurrentScore);

	uint Level_w, Level_h,
		Lives_w, Lives_h,
		Status_w, Status_h,
		Score_w, Score_h;

	pFont->GetTextDimensions(chLevel, Level_w, Level_h);
	pFont->GetTextDimensions(chLives, Lives_w, Lives_h);
	pFont->GetTextDimensions(chStatus, Status_w, Status_h);
	pFont->GetTextDimensions(chScore, Score_w, Score_h);

	pFont->Draw2DSimple((SCREEN_WIDTH - Level_w) / 8, Level_h * 3 / 4, chLevel, ColorDarkGreen());
	pFont->Draw2DSimple((SCREEN_WIDTH - Lives_w) * 3 / 8, Lives_h * 3 / 4, chLives, ColorRed());
	if (CurrentLevel == 1 && Lives == 3 || IsBlackFlagSet())
		pFont->Draw2DSimple((SCREEN_WIDTH - Status_w) * 5 / 8, Status_h * 3 / 4, chStatus, ColorRed());
	pFont->Draw2DSimple((SCREEN_WIDTH - Score_w) * 7 / 8, Score_h * 3 / 4, chScore, ColorBlue());

	//pRender2D->End2D();

	return S_OK;
}

DGLE_RESULT DGLE_API Game::OnEvent(E_EVENT_TYPE eEventType, IBaseEvent *pEvent)
{
	return S_OK;
}