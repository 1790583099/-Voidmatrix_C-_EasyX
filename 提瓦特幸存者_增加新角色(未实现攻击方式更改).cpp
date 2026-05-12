#include <graphics.h>
#include <string>
#include <vector>



int idx_current_anim = 0;
int health = 3;

const int PAIMON_ANIM_NUM = 6;
const int WARRIOR_ANIM_WALK_NUM = 8;
const int WARRIOR_ANIM_IDLE_NUM = 4;

const int ENEMY_ANIM_NUM = 6;
const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;
const int BUTTON_WIDTH = 192;
const int BUTTON_HEIGHT = 75;

const int CHARACTER_WIDTH = 84;
const int CHARACTER_HEIGHT = 84;

bool character_paimon = false;
bool character_warrior = false;
bool is_character_selected = false;
bool is_character_moving = false;
bool is_character_attacking = false;


bool is_game_started = false;
bool is_game_exited = false;

#pragma comment(lib,"Winmm.lib")
#pragma comment(lib,"MSIMG32.LIB")

inline void putimage_alpha(int x, int y, IMAGE* img) {

	int w = img->getwidth();

	int h = img->getheight();

	AlphaBlend(GetImageHDC(NULL), x, y, w, h,

		GetImageHDC(img), 0, 0, w, h, { AC_SRC_OVER,0,255,AC_SRC_ALPHA });

}

class Atlas
{
public:
	Atlas(LPCTSTR path, int num)
	{
		TCHAR path_file[256];
		for (size_t i = 0; i < num; i++)
		{
			_stprintf_s(path_file, path, i);
			IMAGE* frame = new IMAGE();
			loadimage(frame, path_file);
			frame_list.push_back(frame);
		}
	}
	~Atlas()
	{
		for (size_t i = 0; i < frame_list.size(); i++)
		{
			delete frame_list[i];
		}
	}

public:
	std::vector<IMAGE*> frame_list;
};

Atlas* atlas_paimon_left; // 放在main中初始化
Atlas* atlas_paimon_right;
Atlas* atlas_enemy_left;
Atlas* atlas_enemy_right;
Atlas* atlas_warrior_left; // 放在main中初始化
Atlas* atlas_warrior_right;
Atlas* atlas_warrior_idle_left; // 放在main中初始化
Atlas* atlas_warrior_idle_right;


class Animation
{
public:

	Animation(Atlas* atlas, int interval)
	{
		anim_atlas = atlas;
		interval_ms = interval;
	}

	~Animation() = default;// atlas是Animation类对象共享的公共资产，所以不能在Animation的析构函数中使用delete释放atlas指针
	// 需要在更上一层释放（main）
	// 况且这里也没有new

	void Play(int x, int y, int delta) {
		timer += delta;  // 累加时间

		// 如果累加时间超过帧间隔，切换到下一帧
		if (timer >= interval_ms) {
			// 切换到下一帧（使用取模运算实现循环播放）
			idx_frame = (idx_frame + 1) % anim_atlas->frame_list.size();
			timer = 0;  // 重置计时器
		}

		// 在指定位置绘制当前帧
		putimage_alpha(x, y, anim_atlas->frame_list[idx_frame]);
	}

private:

	int timer = 0;
	int idx_frame = 0;
	int interval_ms = 0;

private:
	Atlas* anim_atlas; // 需要持有Atlas类的指针
};


class Bullet
{
public:
	Bullet() = default;
	~Bullet() = default;

	void Draw() const
	{
		setlinecolor(RGB(255, 155, 50));
		setfillcolor(RGB(200, 75, 10));
		fillcircle(position.x, position.y, RADIUS);
	}

	int GetRadius() const { return RADIUS; }

private:
	const int RADIUS = 10;
public:
	POINT position = { 0,0 };
};

class Character
{
public:
	Character()
	{
		loadimage(&img_shadow, _T("img/shadow_player.png"));
	}
	virtual ~Character() = default;  // 虚析构保证派生类正确释放

	void ProcessEvent(const ExMessage& msg)
	{
		switch (msg.message)
		{
		case WM_KEYDOWN:
			switch (msg.vkcode)
			{
			case VK_UP:    is_moving_up = true;    break;
			case VK_DOWN:  is_moving_down = true;  break;
			case VK_LEFT:  is_moving_left = true;  break;
			case VK_RIGHT: is_moving_right = true; break;
			}
			break;
		case WM_KEYUP:
			switch (msg.vkcode)
			{
			case VK_UP:    is_moving_up = false;    break;
			case VK_DOWN:  is_moving_down = false;  break;
			case VK_LEFT:  is_moving_left = false;  break;
			case VK_RIGHT: is_moving_right = false; break;
			}
			break;
		}
	}

	void Move()
	{
		int dir_x = is_moving_right - is_moving_left;
		int dir_y = is_moving_down - is_moving_up;
		double len = sqrt(dir_x * dir_x + dir_y * dir_y);
		if (len != 0)
		{
			double norm_x = dir_x / len;
			double norm_y = dir_y / len;
			pos.x += SPEED * norm_x;
			pos.y += SPEED * norm_y;
		}

		// 边界限制
		if (pos.x < 0) pos.x = 0;
		if (pos.y < 0) pos.y = 0;
		if (pos.x + WIDTH > WINDOW_WIDTH)  pos.x = WINDOW_WIDTH - WIDTH;
		if (pos.y + HEIGHT > WINDOW_HEIGHT) pos.y = WINDOW_HEIGHT - HEIGHT;
	}

	POINT GetPosition() const { return pos; }
	int GetWidth()      const { return WIDTH; }
	int GetHeight()     const { return HEIGHT; }

	virtual void Draw(int delta) = 0;  // 纯虚函数，派生类必须实现

protected:

	static constexpr int SPEED = 3;
	static constexpr int WIDTH = 80;
	static constexpr int HEIGHT = 80;
	const int SHADOW_WIDTH = 32;
	POINT pos = { 500, 500 };
	bool is_moving_up = false, is_moving_down = false;
	bool is_moving_left = false, is_moving_right = false;

	IMAGE img_shadow;
	bool facing_left = false;  // 当前面向方向
};

class Paimon : public Character
{
public:
	Paimon()
	{
		anim_left = new Animation(atlas_paimon_left, 45);
		anim_right = new Animation(atlas_paimon_right, 45);
	}
	~Paimon()
	{
		delete anim_left;
		delete anim_right;
	}
	void Draw(int delta) override
	{
		// 绘制阴影
		int shadow_x = pos.x + (WIDTH / 2 - SHADOW_WIDTH / 2);
		int shadow_y = pos.y + HEIGHT - 8;
		putimage_alpha(shadow_x, shadow_y, &img_shadow);

		// 更新面向方向
		if (is_moving_right - is_moving_left < 0)
			facing_left = true;
		else if (is_moving_right - is_moving_left > 0)
			facing_left = false;

		// 播放对应奔跑动画
		if (facing_left)
			anim_left->Play(pos.x, pos.y, delta);
		else
			anim_right->Play(pos.x, pos.y, delta);
	}

private:
	Animation* anim_left = nullptr;
	Animation* anim_right = nullptr;
};
class Warrior : public Character
{
public:
	Warrior()
	{
		anim_left = new Animation(atlas_warrior_left, 45);
		anim_right = new Animation(atlas_warrior_right, 45);
		anim_idle_left = new Animation(atlas_warrior_idle_left, 45);
		anim_idle_right = new Animation(atlas_warrior_idle_right, 45);
	}
	~Warrior()
	{
		delete anim_left;
		delete anim_right;
		delete anim_idle_left;
		delete anim_idle_right;
	}
	void Draw(int delta) override
	{
		// 绘制阴影
		int shadow_x = pos.x + (WIDTH / 2 - SHADOW_WIDTH / 2);
		int shadow_y = pos.y + HEIGHT - 8;
		// 更新面向方向
		if (is_moving_right - is_moving_left < 0)
			facing_left = true;
		else if (is_moving_right - is_moving_left > 0)
			facing_left = false;

		is_move = is_moving_right||is_moving_left||is_moving_up||is_moving_down;

		// 播放对应奔跑动画
		if (facing_left)
		{
			if (is_move)
			{
				anim_left->Play(pos.x, pos.y, delta);
			}
			else
			{
				anim_idle_left->Play(pos.x, pos.y, delta);
			}
		}
		else
		{
			if (is_move)
			{
				anim_right->Play(pos.x, pos.y, delta);
			}
			else
			{
				anim_idle_right->Play(pos.x, pos.y, delta);
			}
		}
	}



private:
	Animation* anim_left = nullptr;
	Animation* anim_right = nullptr;
	Animation* anim_idle_left = nullptr;
	Animation* anim_idle_right = nullptr;
	bool is_move = false;
};


class Enemy
{
public:
	Enemy()
	{
		loadimage(&img_shadow, _T("img/shadow_enemy.png"));
		anim_enemy_left = new Animation(atlas_enemy_left, 45);
		anim_enemy_right = new Animation(atlas_enemy_right, 45);


		enum class SpawnEdge
		{
			Up = 0,
			Down = 1,
			Left = 2,
			Right = 3,
		};

		SpawnEdge spawn_edge = (SpawnEdge)(rand() % 4);
		switch (spawn_edge)
		{
		case SpawnEdge::Up:
			// 从屏幕**顶部外**随机水平位置出现
			enemy_pos.x = rand() % (WINDOW_WIDTH - ENEMY_WIDTH);
			enemy_pos.y = -ENEMY_HEIGHT;
			break;
		case SpawnEdge::Down:
			// 从屏幕**底部外**随机水平位置出现
			enemy_pos.x = rand() % (WINDOW_WIDTH - ENEMY_WIDTH);
			enemy_pos.y = WINDOW_HEIGHT;
			break;
		case SpawnEdge::Left:
			// 从屏幕**左侧外**随机垂直位置出现
			enemy_pos.x = -ENEMY_WIDTH;
			enemy_pos.y = rand() % (WINDOW_HEIGHT - ENEMY_HEIGHT);
			break;
		case SpawnEdge::Right:
			// 从屏幕**右侧外**随机垂直位置出现
			enemy_pos.x = WINDOW_WIDTH;
			enemy_pos.y = rand() % (WINDOW_HEIGHT - ENEMY_HEIGHT);
			break;
		default:
			break;
		}
	}
	bool CheckBulletCollision(const Bullet& bullet)
	{
		// 1. 敌人的矩形范围（整个身体）
		int eLeft = enemy_pos.x + ENEMY_WIDTH / 4;;
		int eRight = enemy_pos.x + ENEMY_WIDTH / 4 * 3;
		int eTop = enemy_pos.y + ENEMY_HEIGHT / 4;
		int eBottom = enemy_pos.y + ENEMY_HEIGHT / 4 * 3;

		// 2. 子弹的圆心 + 半径
		int bx = bullet.position.x;
		int by = bullet.position.y;
		int br = bullet.GetRadius();

		// 3. 核心：矩形 + 圆形 碰撞判定（游戏标准写法）
		bool hitX = (bx + br >= eLeft) && (bx - br <= eRight);
		bool hitY = (by + br >= eTop) && (by - br <= eBottom);

		return hitX && hitY;
	}

	bool CheckPlayerCollision(const Character& player)
	{
		POINT check_position = { enemy_pos.x + ENEMY_WIDTH / 2,enemy_pos.y + ENEMY_HEIGHT / 2 };
		bool is_overlap_x = check_position.x >= player.GetPosition().x && check_position.x <= player.GetPosition().x + player.GetWidth();
		bool is_overlap_y = check_position.y >= player.GetPosition().y && check_position.y <= player.GetPosition().y + player.GetHeight();
		return is_overlap_x && is_overlap_y;
	}

	void Move(const Character& player)
	{
		const POINT& player_pos = player.GetPosition();
		int dir_x = player_pos.x - enemy_pos.x;
		int dir_y = player_pos.y - enemy_pos.y;
		is_move = dir_x;
		double len_dir = sqrt(dir_x * dir_x + dir_y * dir_y);
		if (len_dir != 0)
		{
			double normalized_dir_x = dir_x / len_dir;
			double normalized_dir_y = dir_y / len_dir;
			enemy_pos.x += ENEMY_SPEED * normalized_dir_x;
			enemy_pos.y += ENEMY_SPEED * normalized_dir_y;
		}
	}

	void Draw(int delta)
	{
		int pos_shadow_x = enemy_pos.x + (ENEMY_WIDTH / 2 - SHADOW_WIDTH / 2);

		int pos_shadow_y = enemy_pos.y + ENEMY_HEIGHT - 25;

		putimage_alpha(pos_shadow_x, pos_shadow_y, &img_shadow);

		static bool facing_left = false;

		if (is_move < 0)

			facing_left = true;

		else if (is_move > 0)

			facing_left = false;

		if (facing_left)

			anim_enemy_left->Play(enemy_pos.x, enemy_pos.y, delta);

		else

			anim_enemy_right->Play(enemy_pos.x, enemy_pos.y, delta);
	}

	void Hurt()
	{
		alive = false;
	}
	bool CheckAlive() const
	{
		return alive;
	}

	~Enemy()
	{
		delete anim_enemy_left;
		delete anim_enemy_right;
	}

private:
	const int ENEMY_SPEED = 2;
	const int ENEMY_WIDTH = 80;

	const int ENEMY_HEIGHT = 80;

	const int SHADOW_WIDTH = 48;

	IMAGE img_shadow;
	POINT enemy_pos = { 500,500 };

	Animation* anim_enemy_left;
	Animation* anim_enemy_right;
	bool facing_left = false;
	bool alive = true;

	bool is_moving_up = false;
	bool is_moving_down = false;
	bool is_moving_left = false;
	bool is_moving_right = false;
	int is_move = 0;
};


class Button
{
public:
	Button(RECT rect, LPCTSTR path_imag_idle, LPCTSTR path_imag_hovered, LPCTSTR path_imag_pushed) // 加载图片
	{
		region = rect;
		loadimage(&img_idle, path_imag_idle);
		loadimage(&img_hovered, path_imag_hovered);
		loadimage(&img_pushed, path_imag_pushed);
	}

	~Button() = default;

	void Draw()
	{
		switch (status)
		{
		case Status::Idle:
			putimage(region.left, region.top, &img_idle);
			break;
		case Status::Hovered:
			putimage(region.left, region.top, &img_hovered);
			break;
		case Status::Pushed:
			putimage(region.left, region.top, &img_pushed);
			break;
		}
	}

	void ProcessEvent(const ExMessage& msg)
	{
		switch (msg.message)
		{
		case WM_MOUSEMOVE:
			if (status == Status::Idle && CheckCursorHit(msg.x, msg.y))
				status = Status::Hovered;
			else if (status == Status::Idle && !CheckCursorHit(msg.x, msg.y))
				status = Status::Idle;
			else if (status == Status::Hovered && !CheckCursorHit(msg.x, msg.y))
				status = Status::Idle;
			break;
		case WM_LBUTTONDOWN:
			if (CheckCursorHit(msg.x, msg.y))
				status = Status::Pushed;
			break;
		case WM_LBUTTONUP:
			if (status == Status::Pushed)
				OnClick();
			break;
		default:
			break;
		}
	}

protected:
	virtual void OnClick() = 0;

private:
	bool CheckCursorHit(int x, int y)
	{
		return x >= region.left && x <= region.right && y >= region.top && y <= region.bottom;
	}

private:
	enum class Status
	{
		Idle = 0,
		Hovered,
		Pushed
	};

private:
	RECT region;
	IMAGE img_idle;
	IMAGE img_hovered;
	IMAGE img_pushed;
	Status status = Status::Idle;
};

class StartGameButton : public Button
{
public:
	StartGameButton(RECT rect, LPCTSTR path_imag_idle, LPCTSTR path_imag_hovered, LPCTSTR path_imag_pushed)
		: Button(rect, path_imag_idle, path_imag_hovered, path_imag_pushed) {
	}
	~StartGameButton() = default;

protected:
	void OnClick()
	{
		is_game_started = true;
		mciSendString(_T("play bgm repeat from 0"), NULL, 0, NULL); // 重复播放bgm
	}
};
class ExitGameButton : public Button
{
public:
	ExitGameButton(RECT rect, LPCTSTR path_imag_idle, LPCTSTR path_imag_hovered, LPCTSTR path_imag_pushed)
		: Button(rect, path_imag_idle, path_imag_hovered, path_imag_pushed) {
	}
	~ExitGameButton() = default;

protected:
	void OnClick()
	{
		is_game_exited = true;
	}
};

void TryGenerateEnemy(std::vector<Enemy*>& enemies)
{
	const int INTERVAL = 100;
	static int counter = 0;
	if (++counter % INTERVAL == 0)
	{
		enemies.push_back(new Enemy());
	}
}

void UpdateBullets(std::vector<Bullet>& bullets, const Character& player)
{
	const double RADIAL_SPEED = 0.0045;
	const double TANGET_SPEED = 0.0055;
	double radian_interval = 2 * 3.14159 / bullets.size();
	POINT player_position = player.GetPosition();
	double radius = 100 + 25 * sin(GetTickCount() * RADIAL_SPEED);
	for (size_t i = 0; i < bullets.size(); i++)
	{
		double radian = GetTickCount() * TANGET_SPEED + i * radian_interval;
		bullets[i].position.x = player_position.x + player.GetWidth() / 2 + radius * sin(radian);
		bullets[i].position.y = player_position.y + player.GetHeight() / 2 + radius * cos(radian);
	}
}

void DrawPlayerScore(int score)
{
	static TCHAR text[64];
	_stprintf_s(text, _T("当前玩家得分: %d"), score);

	setbkmode(TRANSPARENT);
	settextcolor(RGB(255, 85, 185));
	outtextxy(10, 10, text);
}


class SelectCarator
{
public:
	SelectCarator(POINT point, LPCTSTR path_img_character)
	{
		this->point = point;
		loadimage(&img_character, path_img_character);
	}

	~SelectCarator() = default;

	void Draw()
	{
		putimage(point.x, point.y, &img_character);
		if (hangover)
		{
			setlinecolor(RED);
			setlinestyle(PS_SOLID, 2);
			rectangle(point.x-2, point.y-2, point.x + CHARACTER_WIDTH+2, point.y + CHARACTER_HEIGHT+2);
		}
	}

	void ProcessEvent(const ExMessage& msg)
	{
		switch (msg.message)
		{
		case WM_MOUSEMOVE:
			if(CheckRegion(msg.x, msg.y))
			{
				hangover = true;
			}
			else
			{
				hangover = false;
			}
			break;
		case WM_LBUTTONDOWN:
			if (CheckRegion(msg.x, msg.y))
			{
				selected = true;
			}
			break;
		case WM_LBUTTONUP:
			if (selected)
			{
				OnClick();
				selected = false;
			}
			break;
		default:
			break;
		}
	}

protected:
	virtual void OnClick() = 0;
private:
	bool CheckRegion(int x, int y)
	{
		return x >= (point.x) && x <= (point.x + CHARACTER_WIDTH) && y >= (point.y) && y <= (point.y + CHARACTER_HEIGHT);
	}
private:
	POINT point;
	IMAGE img_character;
	bool hangover = false;
	bool selected = false;
};

class PaimonButton : public SelectCarator
{
public:
	PaimonButton(POINT point, LPCTSTR path_img_character)
		: SelectCarator(point, path_img_character) {}
	~PaimonButton() = default;

protected:
	void OnClick()
	{
		character_paimon = true;
		is_character_selected = true;
	}
};

class WarriorButton : public SelectCarator
{
public:
	WarriorButton(POINT point, LPCTSTR path_img_character)
		: SelectCarator(point, path_img_character) {
	}
	~WarriorButton() = default;

protected:
	void OnClick()
	{
		character_warrior = true;
		is_character_selected = true;
	}
};
void RunGameLogic(Character& player, std::vector<Enemy*>& enemies, std::vector<Bullet>& bullets, int& score, bool& running)
{
	// 移动玩家
	player.Move();

	// 更新环绕子弹的位置
	UpdateBullets(bullets, player);

	// 尝试生成新敌人
	TryGenerateEnemy(enemies);

	// 所有敌人向玩家移动
	for (Enemy* enemy : enemies)
		enemy->Move(player);

	// 玩家与敌人的碰撞（游戏结束）
	for (Enemy* enemy : enemies)
	{
		if (enemy->CheckPlayerCollision(player))
		{
			MessageBox(GetHWnd(), _T("Game Over"), _T("Game Over"), MB_OK);
			running = false;
			return;  // 直接结束本帧，不再继续处理
		}
	}

	// 子弹与敌人的碰撞（伤害+得分）
	for (Enemy* enemy : enemies)
	{
		for (Bullet& bullet : bullets)
		{
			if (enemy->CheckBulletCollision(bullet))
			{
				mciSendString(_T("play hit from 0"), NULL, 0, NULL);
				enemy->Hurt();
				score++;
				// 注意：一个子弹可能同时击中多个敌人，但这里每个子弹每帧只伤害一个敌人，可以 break 内层循环（可选）
				break;
			}
		}
	}

	// 移除死亡敌人（交换并删除）
	for (size_t i = 0; i < enemies.size(); )
	{
		Enemy* enemy = enemies[i];
		if (!enemy->CheckAlive())
		{
			std::swap(enemies[i], enemies.back());
			enemies.pop_back();
			delete enemy;
			// 注意：此时 i 不变，继续检查新的 enemies[i]
		}
		else
		{
			i++;
		}
	}
}

int main() {

	initgraph(WINDOW_WIDTH, WINDOW_HEIGHT);
	atlas_paimon_left = new Atlas(_T("img/paimon_left_%d.png"), PAIMON_ANIM_NUM);
	atlas_paimon_right = new Atlas(_T("img/paimon_right_%d.png"), PAIMON_ANIM_NUM);
	atlas_warrior_left = new Atlas(_T("img/warrior_walk_left_%d.png"), WARRIOR_ANIM_WALK_NUM);
	atlas_warrior_right = new Atlas(_T("img/warrior_walk_right_%d.png"), WARRIOR_ANIM_WALK_NUM);
	atlas_warrior_idle_left = new Atlas(_T("img/warrior_idle_left_%d.png"), WARRIOR_ANIM_IDLE_NUM);
	atlas_warrior_idle_right = new Atlas(_T("img/warrior_idle_right_%d.png"), WARRIOR_ANIM_IDLE_NUM);
	atlas_enemy_left = new Atlas(_T("img/boar_left_%d.png"), ENEMY_ANIM_NUM);
	atlas_enemy_right = new Atlas(_T("img/boar_right_%d.png"), ENEMY_ANIM_NUM);

	mciSendString(_T("open mus/bgm.mp3 alias bgm"), NULL, 0, NULL);
	mciSendString(_T("open mus/hit.wav alias hit"), NULL, 0, NULL);

	Paimon paimon = Paimon();
	Warrior warrior = Warrior();
	PaimonButton paimon_button = PaimonButton({ 300,500 }, _T("img/avatar_paimon.png"));
	WarriorButton warrior_button = WarriorButton({980,500 }, _T("img/avatar_warrior.png"));
	bool running = true;

	RECT region_btn_start_game, region_btn_quit_game;
	region_btn_start_game.left = (WINDOW_WIDTH - BUTTON_WIDTH) / 2;
	region_btn_start_game.right = region_btn_start_game.left + BUTTON_WIDTH;
	region_btn_start_game.top = 430;
	region_btn_start_game.bottom = region_btn_start_game.top + BUTTON_HEIGHT;

	region_btn_quit_game.left = (WINDOW_WIDTH - BUTTON_WIDTH) / 2;
	region_btn_quit_game.right = region_btn_quit_game.left + BUTTON_WIDTH;
	region_btn_quit_game.top = 550;
	region_btn_quit_game.bottom = region_btn_quit_game.top + BUTTON_HEIGHT;
	StartGameButton btn_start_game = StartGameButton(region_btn_start_game,
		_T("img/ui_start_idle.png"), _T("img/ui_start_hovered.png"), _T("img/ui_start_pushed.png"));

	ExitGameButton btn_quit_game = ExitGameButton(region_btn_quit_game,
		_T("img/ui_quit_idle.png"), _T("img/ui_quit_hovered.png"), _T("img/ui_quit_pushed.png"));


	int score = 0;
	ExMessage msg;
	IMAGE img_menu;
	IMAGE img_background;
	std::vector<Enemy*> enemies;
	std::vector<Bullet> bullets(3);
	loadimage(&img_background, _T("img/background.png"));
	loadimage(&img_menu, _T("img/menu.png"));
	BeginBatchDraw();

	while (running) {
		DWORD start_time = GetTickCount();
		while (peekmessage(&msg, EX_MOUSE | EX_KEY))
		{
			if (is_game_started == false)
			{
				btn_start_game.ProcessEvent(msg);
				btn_quit_game.ProcessEvent(msg);
				if (is_game_exited == true)
				{
					running = false;
				}
			}
			else if (is_game_started == true&&is_character_selected == true)
			{
				if (character_warrior)
				{
					warrior.ProcessEvent(msg);
				}
				else if (character_paimon)
				{
					paimon.ProcessEvent(msg);
				}
			}
			else if (is_game_started == true&&is_character_selected == false)
			{
				paimon_button.ProcessEvent(msg);
				warrior_button.ProcessEvent(msg);
			}
			
		}
		if (is_game_started == true && is_character_selected == true)
		{
			if (character_warrior)
				RunGameLogic(warrior, enemies, bullets, score, running);
			else if (character_paimon)
				RunGameLogic(paimon, enemies, bullets, score, running);
		}
		cleardevice();
		if (is_game_started == false)
		{
			putimage(0, 0, &img_menu);
			btn_start_game.Draw();
			btn_quit_game.Draw();

		}
		else if (is_game_started == true && is_character_selected == true)
		{
			putimage(0, 0, &img_background);
			DrawPlayerScore(score);
			if (character_warrior)
				warrior.Draw(1000 / 144);
			else if (character_paimon)
				paimon.Draw(1000 / 144);
			for (Enemy* enemy : enemies)
			{
				enemy->Draw(1000 / 144);
			}
			for (const Bullet& bullet : bullets)
			{
				bullet.Draw();
			}

			DWORD end_time = GetTickCount();
			DWORD delta_time = end_time - start_time;
			if (delta_time < 1000 / 144) {
				Sleep(1000 / 144 - delta_time);
			}
		}
		else
		{
			putimage(0, 0, &img_menu);
			paimon_button.Draw();
			warrior_button.Draw();
		}
		FlushBatchDraw();
	}
	delete atlas_paimon_left;
	delete atlas_paimon_right;
	delete atlas_warrior_left;
	delete atlas_warrior_right;
	delete atlas_warrior_idle_left;
	delete atlas_warrior_idle_right;
	delete atlas_enemy_left;
	delete atlas_enemy_right;
	EndBatchDraw();

	return 0;

}
