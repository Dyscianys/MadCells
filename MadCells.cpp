#include<raylib.h>
#include<raymath.h>
#include<vector>
#include<queue>
#include<iostream>
#include<cmath>
#include<random>
using namespace std;

const int MAX_BULLETS=200;
const int SCREEN_WIDTH=2400;
const int SCREEN_HEIGHT=1400;
const int CONNECT_INDEX=7;
const int BLOCK_ALLTYPE=11;

std::random_device rd;//随机数用
std::mt19937 rng(rd());
int RandomInt(int min,int max) {
	std::uniform_int_distribution<int> dist(min,max);
	return dist(rng);
}
float RandomFloat(float min,float max) {
	std::uniform_real_distribution<float> dist(min,max);
	return dist(rng);
}

enum class GameState
{
	GAMING,ASSEMBLING,MEMU,CREATER
};
enum class Type
{
	player,chlorella
};
enum class PartType
{
	none,cy_0,cy_1,cy_2c,cy_2p,cy_3,cy_4,//cy->cytosol细胞质
	mount,//鞭毛
	cilium_left,cilium_right,//纤毛
	spike//刺细胞
};
enum class Organelle
{
	none,nucleus,mitochondrion,specialized_oral_groove
};
enum class Blockstate
{
	none,active
};
enum class Bullettype
{
	foodvacuole,spike
};
enum class Particletype
{
	bubble,hit,geometricslime,deadring,overloadring
};
enum class Dropparttype
{
	sixside,circle
};
enum class Droptype
{
	sugermonomer,sugerdimer,sugerpolymer
};

GameState interface=GameState::CREATER;

struct Sound2D
{
	Sound alias;
	bool active=false;
	Sound* basesound=nullptr;
};
struct SoundPool
{
	vector<Sound2D> Sounds;
	void init(Sound& base,int count)
	{
		for(int i=0;i<count;i++)
		{
			Sound2D newsound;
			newsound.alias=LoadSoundAlias(base);
			newsound.basesound=&base;
			newsound.active=false;
			Sounds.push_back(newsound);
		}
	}
	bool play(Sound& base,float volume=1.0f)
	{
		for(auto& it:Sounds)
		{
			if(!it.active && it.basesound==&base)
			{
				SetSoundVolume(it.alias,volume);
				PlaySound(it.alias);
				it.active=true;
				return true;
			}
		}
		return false;
	}
	void update()
	{
		for(auto& it:Sounds)
		{
			if(it.active && !IsSoundPlaying(it.alias))
			{
				it.active=false;
			}
		}
	}
};

struct Particle
{
	Particletype type;
	Vector2 position;
	Vector2 velocity;
	float vd=0.0f,direction=0.0f;
	float size,size2;
	float maxsize;
	Rectangle source;
	Color color1,color2;
	float lifetime=0.0f;
	float timer=0.0f;
	Vector2 pos2;
	int side;
	float alpha;
	void update(float deltaT)
	{
		lifetime-=deltaT;
		switch(type)
		{
		case Particletype::bubble:
			velocity*=0.93;
			size+=(maxsize-size)*0.1f;
			break;
		case Particletype::hit:
			velocity*=0.95;
			size+=(0-size)*0.15f;
			break;
		case Particletype::geometricslime:
			velocity*=0.98;
			vd*=0.98f;
			direction+=vd*deltaT;
			size+=(maxsize-size)*0.04f;
			alpha+=(0-alpha)*0.005f;
			color1.a=alpha;
			if(alpha>=5.0f) lifetime=1e6;
			else lifetime=-1.0f;
			break;
		case Particletype::deadring:
			size+=(maxsize-size)*0.18f;
			size2+=(maxsize-size2)*0.08f;
			if(maxsize-size2>=3.0f) lifetime=1e6;
			else lifetime=-1.0f;
			break;
		case Particletype::overloadring:
			size+=(maxsize-size)*0.13f;
			size2+=(maxsize-size2)*0.05f;
			if(maxsize-size2>=3.0f) lifetime=1e6;
			else lifetime=-1.0f;
			break;
		default:
			break;
		}
		position+=velocity*deltaT;
	}
	void draw()
	{
		switch(type)
		{
		case Particletype::bubble:
		{
			float radius=size/2.0f;
			DrawCircleV(position,radius,color2);
			DrawRing(position,radius,radius+3.0f,0,360,24,color1);
			Vector2 highlight=position-Vector2{radius*0.6f,radius*0.6f};
			DrawCircleV(highlight,3.0f,{255,255,255,50});
			break;
		}
		case Particletype::hit:
		{
			Rectangle rect={position.x,position.y-4,size,8};
			DrawRectanglePro(rect,{0,0},direction,color1);
			break;
		}
		case Particletype::geometricslime:
		{
			float radius=size/2.0f;
			Color c=color1;
			c.a*=1.3;
			DrawPoly(position,side,radius,direction,color1);
			DrawPolyLinesEx(position,side,radius,direction,3.0f,c);
			break;
		}
		case Particletype::deadring:
			DrawRing(position,(size/2.0f),(size2/2.0f),0,360,24,color1);
			break;
		case Particletype::overloadring:
			DrawRing(position,(size/2.0f),(size2/2.0f),0,360,24,color1);
			break;
		default:
			break;
		}
	}
};
struct Particlemaster
{
	vector<Particle> Particles;
	void createparticle(Particletype type,Vector2 pos,Vector2 vel,float dir,Color color,float size,int num)
	{
		for(int i=0;i<num;i++)
		{
			Particle newParticle;
			newParticle.type=type;
			newParticle.position=pos;
			switch(type)
			{
			case Particletype::bubble:
				newParticle.velocity.x=vel.x*RandomFloat(0.2f,1.8f);
				newParticle.velocity.y=vel.y*RandomFloat(0.2f,1.8f);
				newParticle.size=0.0f;
				newParticle.maxsize=RandomFloat(5.0f,80.0f);
				newParticle.lifetime=RandomFloat(3.0f,5.0f);
				newParticle.color1={255,255,255,70};
				newParticle.color2={255,255,255,20};
				break;
			case Particletype::hit:
				newParticle.size=RandomFloat(60.0f,100.0f);
				newParticle.direction=dir+180+RandomFloat(-60.0f,60.0f);
				newParticle.velocity.x=cosf(newParticle.direction*DEG2RAD)*300;
				newParticle.velocity.y=sinf(newParticle.direction*DEG2RAD)*300;
				newParticle.lifetime=2.0f;
				newParticle.color1={255,255,0,255};
				break;
			case Particletype::geometricslime:
				newParticle.velocity.x=RandomFloat(vel.x,vel.y);
				newParticle.velocity.y=RandomFloat(vel.x,vel.y);
				newParticle.vd=RandomFloat(-60.0f,60.0f);
				newParticle.direction=RandomFloat(0.0f,360.0f);
				newParticle.size=0.0f;
				newParticle.maxsize=RandomFloat(50.0f,size);
				if(RandomInt(1,3)>2) newParticle.color1=color;
				else newParticle.color1={255,255,255,150};
				newParticle.side=RandomInt(3,8);
				newParticle.alpha=RandomFloat(100.0f,200.0f);
				break;
			case Particletype::deadring:
				newParticle.velocity={0,0};
				newParticle.size=0.0f;
				newParticle.size2=0.0f;
				newParticle.maxsize=size;
				newParticle.color1=color;
				break;
			case Particletype::overloadring:
				newParticle.velocity={0,0};
				newParticle.size=0.0f;
				newParticle.size2=0.0f;
				newParticle.maxsize=size;
				newParticle.color1=color;
				break;
			default:
				break;
			}
			Particles.push_back(newParticle);
		}
	}
};
Particlemaster Particle_master;

struct Droppart
{
	Dropparttype type;
	Vector2 local;
	Vector2 rocate;
	float size;
	Color color;
};
struct Drop
{
	Droptype type;
	Vector2 position={0};
	Vector2 velocity={0};
	float direction=0.0f;
	float vd=30.0f;
	float lifetime=30.0f;
	bool active=true;
	vector<Droppart> Parts;
	Drop(Droptype t,Vector2 pos,Vector2 vel,float d)
	{
		type=t;
		position=pos;
		velocity=vel;
		direction=d;
		Droppart newPart;
		switch(t)
		{
		case Droptype::sugermonomer:
			newPart.type=Dropparttype::sixside;
			newPart.local={0,0};
			newPart.size=17.0f;
			newPart.color={255,255,0,255};
			Parts.push_back(newPart);
			break;
		case Droptype::sugerdimer:
			newPart.type=Dropparttype::sixside;
			newPart.local={-7,0};
			newPart.size=17.0f;
			newPart.color={255,150,0,255};
			Parts.push_back(newPart);
			newPart.local={7,0};
			Parts.push_back(newPart);
			break;
		case Droptype::sugerpolymer:
			newPart.type=Dropparttype::sixside;
			newPart.size=17.0f;
			newPart.color={255,50,0,255};
			for(int i=0;i<3;i++)
			{
				newPart.local={RandomFloat(-15.0f,15.0f),RandomFloat(-15.0f,15.0f)};
				Parts.push_back(newPart);
			}
			break;
		default:
			break;
		}
	}
	void update(float deltaT)
	{
		lifetime-=deltaT;
		if(lifetime<=0.0f) active=false;
		velocity*=0.95;
		position+=velocity*deltaT;
		direction+=vd*deltaT;
		float sind=sinf(direction*DEG2RAD);
		float cosd=cosf(direction*DEG2RAD);
		for(auto& it:Parts)
		{
			it.rocate.x=it.local.x*cosd-it.local.y*sind;
			it.rocate.y=it.local.x*sind+it.local.y*cosd;
		}
	}
	void draw()
	{
		for(auto& it:Parts)
		{
			Color c=it.color;c.a=150;
			DrawPoly(position+it.rocate,6,it.size/2.0f,direction,c);
			DrawPolyLinesEx(position+it.rocate,6,it.size/2.0f,direction,3.0f,it.color);
		}
	}
};
struct Dropmaster
{
	vector<Drop> Drops;
	void createdrop(Droptype type,Vector2 pos,float vel,float d,int num)
	{
		for(int i=0;i<num;i++)
		{
			Drop newDrop(type,pos,Vector2{RandomFloat(-vel,vel),RandomFloat(-vel,vel)},d);
			Drops.push_back(newDrop);
		}
	}
	void update(float deltaT)
	{
		for(auto drop=Drops.begin();drop!=Drops.end();)
		{
			drop->update(deltaT);
			if(!drop->active) drop=Drops.erase(drop);
			else drop++;
		}
	}
	void draw()
	{
		for(auto& drop:Drops)
		{
			drop.draw();
		}
	}
};
Dropmaster Drop_master;

struct Block
{
	int belongto_cell_ID;
	Vector2 local={0.0f,0.0f};
	Vector2 rocate={0.0f,0.0f};
	float mass=1.0f;
	float direction=0.0f;
	float organelle_direction=0.0f;
	float hp;
	float timer=RandomFloat(0.5f,1.0f);
	float hittimer=0.0f;
	PartType parttype;
	Organelle organelle;
	Blockstate state=Blockstate::none;
	float charge=0.0f;
	float maxcharge=100.0f;
	bool overload=false;
	float overloadtimer=0.0f;
	float force=0.0f;
	float firetimer=0.0f;
	bool overloadinfluence=false;
	int blockID=0;
	Block(int id,int blockid,Vector2 pos,float m,float hp_,PartType ma,Organelle o)
	{
		belongto_cell_ID=id;
		blockID=blockid;
		local=pos*32;
		mass=m;
		hp=hp_;
		parttype=ma;
		organelle=o;
	}
	void Drawlight(Texture2D texture,PartType parttype,Organelle organelle,Color lightcolor,Vector2 pos,float d,float blo_d,float organ_d)
	{
		switch(parttype)
		{
			case PartType::cy_1:
			DrawTexturePro(texture,{0,128,32,96},{pos.x,pos.y,32,96},{16,112},d+blo_d,lightcolor);
			break;
		case PartType::cy_2p:
			DrawTexturePro(texture,{0,128,32,96},{pos.x,pos.y,32,96},{16,112},d+blo_d,lightcolor);
			DrawTexturePro(texture,{0,128,32,96},{pos.x,pos.y,32,96},{16,112},d+blo_d+180,lightcolor);
			break;
		case PartType::cy_2c:
			DrawTexturePro(texture,{32,128,128,128},{pos.x,pos.y,128,128},{16,112},d+blo_d-90,lightcolor);
			break;
		case PartType::cy_3:
			DrawTexturePro(texture,{160,128,128,224},{pos.x,pos.y,128,224},{16,112},d+blo_d-90,lightcolor);
			break;
		case PartType::cy_4:
			DrawTexturePro(texture,{160,128,128,224},{pos.x,pos.y,128,224},{16,112},d+blo_d-90,lightcolor);
			DrawTexturePro(texture,{160,128,128,224},{pos.x,pos.y,128,224},{16,112},d+blo_d+90,lightcolor);
			break;
		default:
			break;
		}
		switch(organelle)
		{
		case Organelle::specialized_oral_groove:
			DrawTexturePro(texture,{0,352,192,32},{pos.x,pos.y,600,96},{0,48},organ_d,{100,255,100,150});
		default:
			break;
		}
	}
	void Drawblock(Texture2D texture,Type type,Vector2 pos,float d,float blo_d,float organ_d)
	{
		Rectangle source,organ_source;
		Rectangle dest,organ_dest;
		dest={pos.x,pos.y,32,32};
		organ_dest={pos.x,pos.y,32,32};
		Vector2 origin={16,16};
		Color lightcolor;
		bool have_organ=false;
		bool freedire=false;
		switch(type)
		{
		case Type::player:
			lightcolor={0,255,0,50};
			switch(parttype)
			{
			case PartType::cy_0:
				source={32,0,32,32};
				break;
			case PartType::cy_1:
				source={64,0,32,32};
				break;
			case PartType::cy_2p:
				source={96,0,32,32};
				break;
			case PartType::cy_2c:
				source={128,0,32,32};
				break;
			case PartType::cy_3:
				source={160,0,32,32};
				break;
			case PartType::cy_4:
				source={192,0,32,32};
				break;
			case PartType::mount:
				source={224,0,32,32};
				break;
			case PartType::cilium_left:
				source={256,0,32,32};
				break;
			case PartType::cilium_right:
				source={288,0,32,32};
				break;
			default:
				source={0,0,32,32};
				break;
			}
			if(organelle!=Organelle::none)
			{
				have_organ=true;
				switch(organelle)
				{
				case Organelle::nucleus:
					organ_source={32,32,32,32};
					break;
				case Organelle::mitochondrion:
					organ_source={64,32,32,32};
					break;
				case Organelle::specialized_oral_groove:
					organ_source={96,32,64,32};
					organ_dest.width=64;
					freedire=true;
					break;
				default:
					organ_source={0,32,32,32};
					break;
				}
			}
			break;
		case Type::chlorella:
			lightcolor={255,255,0,50};
			switch(parttype)
			{
			case PartType::cy_0:
				source={320,0,32,32};
				break;
			case PartType::cy_1:
				source={352,0,32,32};
				break;
			case PartType::cy_2p:
				source={384,0,32,32};
				break;
			case PartType::cy_2c:
				source={416,0,32,32};
				break;
			case PartType::cy_3:
				source={448,0,32,32};
				break;
			case PartType::cy_4:
				source={480,0,32,32};
				break;
			case PartType::spike:
				source={512,0,32,32};
				break;
			default:
				source={0,0,32,32};
				break;
			}
			if(organelle!=Organelle::none)
			{
				have_organ=true;
				switch(organelle)
				{
				case Organelle::nucleus:
					organ_source={160,32,32,32};
					break;
				default:
					organ_source={0,32,32,32};
					break;
				}
			}
		default:
			break;
		}
		DrawTexturePro(texture,source,dest,origin,d+blo_d,WHITE);
		if(have_organ)
		{
			if(freedire)
			{
				DrawTexturePro(texture,organ_source,organ_dest,origin,organ_d,WHITE);
			}
			else
			{
				DrawTexturePro(texture,organ_source,organ_dest,origin,d+blo_d+organ_d,WHITE);
			}
		}
		Drawlight(texture,parttype,organelle,lightcolor,pos,d,blo_d,organ_d);
		if(overloadinfluence || overload)
		{
			DrawRectanglePro(dest,{16,16},d+blo_d,{255,255,0,150});
		}
		if(hittimer>0.0f)
		{
			DrawRectanglePro({pos.x,pos.y,32,32},{16,16},d,{255,255,255,200});
		}
	}
};

struct Flagellum_point
{
	Vector2 position;
	Flagellum_point(Vector2 pos)
	{
		position=pos;
	}
};

struct Flagellum
{
	int baseblock_index;
	float timer=0.0f;
	vector<Flagellum_point> Flagellum_points;
};

struct Bullet
{
	Vector2 position,velocity;
	float direction;
	float damage;
	float lifetime;
	bool is_active=false;
	Bullettype type;
	int belongto_living_index,belongto_block_index;
	Color lightcolor={255,255,255,255};
};

struct BulletPool
{
	Bullet Bullets[MAX_BULLETS];
	int substeps=5;
	int count=0;
	void fire(Bullettype type,int belongto_living_index,int belongto_block_index,Vector2 pos,float direction,Vector2& shake,float shakeclass)
	{
		shake.x=RandomFloat(-shakeclass,shakeclass);
		shake.y=RandomFloat(-shakeclass,shakeclass);
		int index=-1;
		int oldest_index;
		float oldest_lifetime=1e6,v;
		for(int i=0;i<MAX_BULLETS;i++)
		{
			if(Bullets[i].lifetime<oldest_lifetime)
			{
				oldest_index=i;
				oldest_lifetime=Bullets[i].lifetime;
			}
			if(!Bullets[i].is_active)
			{
				index=i;
				break;
			}
		}
		if(index==-1) index=oldest_index;
		Bullets[index].type=type;
		switch(type)
		{
		case Bullettype::foodvacuole:
			Bullets[index].damage=5.0f;
			Bullets[index].lifetime=10.0f;
			v=50.0f;
			Bullets[index].lightcolor={0,255,0,100};
			break;
		case Bullettype::spike:
			Bullets[index].damage=5.0f;
			Bullets[index].lifetime=5.0f;
			v=50.0f;
			Bullets[index].lightcolor={255,0,0,100};
			break;
		default:
			return;
		}
		Bullets[index].is_active=true;
		count++;
		Bullets[index].belongto_living_index=belongto_living_index;
		Bullets[index].belongto_block_index=belongto_block_index;
		float rad=DEG2RAD*direction;
		Bullets[index].velocity.x=cosf(rad)*v;
		Bullets[index].velocity.y=sinf(rad)*v;
		Bullets[index].position.x=pos.x+cosf(rad)*80;
		Bullets[index].position.y=pos.y+sinf(rad)*80;
		Bullets[index].direction=direction;
		return;
	}
};
BulletPool Bullet_pool;

Vector2 shake;

struct Findblockpickresult
{
	bool is_part;
	bool is_cytosol;
	int block_index;
	bool is_nucleus;
};
int playerID,playernucleusindex;

struct Cell
{
	int id;
	Vector2 velocity={0.0f,0.0f};
	Vector2 position={0.0f,0.0f};
	float vd=0.0f;
	float direction=0.0f;
	Vector2 center={0.0f,0.0f};
	float mass=0.0f;
	float inertia=0.0f;
	Type type;
	float sind;
	float cosd;
	vector<Block> Blocks;
	vector<Flagellum> Flagella;
	Color color;
	bool isdead=false;
	int nextblockID=0;
	float maxenergy=300.0f;
	float energy=100.0f;
	float energydying=false;
	float eneggydyingtimer=0.0f;
	float dyingsoundtimer=0.0f;
	
	void update(float deltaT,Camera2D& gamecamera,SoundPool& Sound_pool,Sound& shoot1,Sound& shoot2,Sound& di)
	{
		velocity=velocity*0.9f;
		vd*=0.9;
		position=position+velocity;
		direction+=vd;
		float rad=DEG2RAD*direction;
		sind=sinf(rad);
		cosd=cosf(rad);
		for(auto& block:Blocks)
		{
			block.rocate.x=(block.local.x-center.x)*cosd-(block.local.y-center.y)*sind;
			block.rocate.y=(block.local.x-center.x)*sind+(block.local.y-center.y)*cosd;
			if(block.timer>0.0f) block.timer-=deltaT;
			if(block.hittimer>0.0f) block.hittimer-=deltaT;
			if(type==Type::player)
			{
				energy-=0.05f*deltaT;
			}
			if(block.state==Blockstate::active)
			{
				switch(block.organelle)
				{
				case Organelle::none:
				{
					float sum_d=direction+block.direction;
					float rad;
					switch(block.parttype)
					{
					case PartType::mount:
						sum_d+=90;
						rad=sum_d*DEG2RAD;
						if(energydying) block.force=60.0f;
						else if(block.overloadinfluence) block.force=360.0f;
						else if(energy<20.0f) block.force=120.0f;
						else block.force=180.0f;
						force(block.rocate,{cosf(rad)*block.force*deltaT,sinf(rad)*block.force*deltaT});
						energy-=0.5*deltaT;
						break;
					case PartType::cilium_left:
						sum_d+=0;
						rad=sum_d*DEG2RAD;
						if(energydying) block.force=30.0f;
						else if(block.overloadinfluence) block.force=160.0f;
						else if(energy<20.0f) block.force=60.0f;
						else block.force=80.0f;
						force(block.rocate,{cosf(rad)*block.force*deltaT,sinf(rad)*block.force*deltaT});
						energy-=0.3f*deltaT;
						break;
					case PartType::cilium_right:
						sum_d+=180;
						rad=sum_d*DEG2RAD;
						if(energydying) block.force=30.0f;
						else if(block.overloadinfluence) block.force=160.0f;
						else if(energy<20.0f) block.force=60.0f;
						else block.force=80.0f;
						force(block.rocate,{cosf(rad)*block.force*deltaT,sinf(rad)*block.force*deltaT});
						energy-=0.3f*deltaT;
						break;
					case PartType::spike:
						if(block.timer<=0.0f)
						{
							float dist=Vector2Distance(position+block.rocate,gamecamera.target)/32.0f;
							Bullet_pool.fire(Bullettype::spike,id,block.blockID,position+block.rocate,direction+block.direction-90,shake,min((15.0f/dist),1.0f)*25.0f);
							float rad=DEG2RAD*(direction+block.direction-90);
							force(block.rocate,{cosf(rad)*-10,sinf(rad)*-10});
							Particle_master.createparticle(Particletype::bubble,position+block.rocate,Vector2{cosf(rad)*500,sinf(rad)*500},0,Color{0,0,0,0},0,RandomInt(2,5));
							Sound_pool.play(shoot2,min(1.0f,15.0f/dist));
							if(block.overloadinfluence) block.firetimer=0.2f;
							else block.firetimer=2.0f+RandomFloat(0.0f,1.0f);
							block.timer=block.firetimer;
						}
						break;
					default:
						break;
					}
					break;
				}
				case Organelle::specialized_oral_groove:
					if(block.timer<=0 && !energydying)
					{
						Bullet_pool.fire(Bullettype::foodvacuole,id,block.blockID,position+block.rocate,block.organelle_direction,shake,20.0f);
						float rad=DEG2RAD*block.organelle_direction;
						force(block.rocate,{cosf(rad)*-10,sinf(rad)*-10});
						Particle_master.createparticle(Particletype::bubble,position+block.rocate,Vector2{cosf(rad)*500,sinf(rad)*500},0,Color{0,0,0,0},0,RandomInt(2,5));
						Sound_pool.play(shoot1,1.0f);
						if(block.overloadinfluence) block.firetimer=0.1f;
						else if(energy<20.0f) block.firetimer=0.3f;
						else block.firetimer=0.2f;
						block.timer=block.firetimer;
						energy-=1.0f;
					}
					break;
				default:
					break;
				}
			}
			block.overloadinfluence=false;
			if(block.overload)
			{
				block.overloadtimer-=deltaT;
				if(block.overloadtimer<=0.0f)
				{
					block.overload=false;
					block.charge=0.0f;
				}
				for(auto& other:Blocks)
				{
					float d=Vector2Distance(block.local,other.local);
					if(d<80.0f)
					{
						other.overloadinfluence=true;
					}
				}
			}
		}
		for(auto& this_flagellum:Flagella)
		{
			if(Blocks[this_flagellum.baseblock_index].state==Blockstate::active)
			{
				this_flagellum.timer+=18*deltaT;
			}
			this_flagellum.timer+=2*deltaT;
			if(this_flagellum.timer>2*PI) this_flagellum.timer-=2*PI;
			this_flagellum.Flagellum_points[0].position.x=position.x+Blocks[this_flagellum.baseblock_index].rocate.x-sind*15*sinf(this_flagellum.timer);
			this_flagellum.Flagellum_points[0].position.y=position.y+Blocks[this_flagellum.baseblock_index].rocate.y+cosd*15*sinf(this_flagellum.timer);
			for(size_t i=1;i<this_flagellum.Flagellum_points.size();i++)
			{
				Vector2 d=this_flagellum.Flagellum_points[i].position-this_flagellum.Flagellum_points[i-1].position;
				float r=sqrtf(d.x*d.x+d.y*d.y);
				this_flagellum.Flagellum_points[i].position=this_flagellum.Flagellum_points[i-1].position+d*25/r;
			}
		}
		if(type==Type::player && playernucleusindex!=-1)
		{
			for(auto drop=Drop_master.Drops.begin();drop!=Drop_master.Drops.end();)
			{
				float dist=Vector2Distance(position+Blocks[playernucleusindex].rocate,drop->position);
				if(dist<20.0f)
				{
					drop=Drop_master.Drops.erase(drop);
					switch(drop->type)
					{
					case Droptype::sugermonomer:
						energy+=3.0f;
						break;
					case Droptype::sugerdimer:
						energy+=5.0f;
						break;
					case Droptype::sugerpolymer:
						energy+=10.0f;
						break;
					default:
						break;
					}
					continue;
				}
				if(dist<130.0f)
				{
					Vector2 delta=(position+Blocks[playernucleusindex].rocate)-drop->position;
					drop->velocity=delta*10.0f;
				}
				drop++;
			}
			if(energy>maxenergy) energy=maxenergy;
		}
		if(energy<=0.0f)
		{
			energy=0.0f;
			energydying=true;
			eneggydyingtimer-=deltaT;
			dyingsoundtimer-=deltaT;
			if(dyingsoundtimer<=0.0f)
			{
				dyingsoundtimer=1.0f;
				Sound_pool.play(di,1.0f);
			}
			if(eneggydyingtimer<=0.0f)
			{
				isdead=true;
			}
		}
		else
		{
			energydying=false;
			eneggydyingtimer=5.0f;
		}
	}
	void found_center()
	{
		if(Blocks.empty())
		{
			center={0.0f,0.0f};
			isdead=true;
			return;
		}
		float sum_mass=0.0f;
		Vector2 center0=center;
		center={0.0f,0.0f};
		mass=0.0f;
		for(const auto& it:Blocks)
		{
			mass+=it.mass;
			center+=it.local*it.mass;
			sum_mass+=it.mass;
		}
		center.x/=sum_mass;
		center.y/=sum_mass;
		position=position+center-center0;
		inertia=0.0f;
		for(const auto& block:Blocks)
		{
			Vector2 d=block.local-center;
			d.x/=7.0f;
			d.y/=7.0f;
			float r_sq=d.x*d.x+d.y*d.y;
			inertia+=block.mass*r_sq;
		}
	}
	bool Check_other_blocks(Vector2 local)
	{
		for(const auto& block:Blocks)
		{
			if(block.local.x==local.x && block.local.y==local.y)
			{
				int index=static_cast<int>(block.parttype);
				return (index>=1 && index<CONNECT_INDEX);
			}
		}
		return false;
	}
	bool need_to_rebuild=true;
	bool BFS_isconnector(Block block)
	{
		int type=(int)block.parttype;
		return (type>=1 && type<CONNECT_INDEX);
	}
	void rebulildcell()
	{
		Flagella.clear();
		int nucleusIndex=-1;
		for(size_t i=0;i<Blocks.size();++i)
			if(Blocks[i].organelle==Organelle::nucleus) {nucleusIndex=i;break;}
		if(nucleusIndex==-1)
		{
			isdead=true;
			return;
		}
		if(type==Type::player)
		{
			playerID=id;
			playernucleusindex=nucleusIndex;
		}
		vector<bool> connected(Blocks.size(), false);
		queue<int> q;
		q.push(nucleusIndex);
		connected[nucleusIndex]=true;
		while(!q.empty())
		{
			int idx=q.front();q.pop();
			Vector2 pos=Blocks[idx].local;
			if(!BFS_isconnector(Blocks[idx])) continue;
			Vector2 dirs[4]={{0,-32},{0,32},{-32,0},{32,0}};
			for(auto& d:dirs)
			{
				Vector2 nPos={pos.x+d.x,pos.y+d.y};
				for (size_t j=0;j<Blocks.size();++j)
				{
					if (!connected[j] && Blocks[j].local==nPos)
					{
						connected[j]=true;
						q.push(j);
					}
				}
			}
		}
		//这是deepseek帮忙修改的↓
		// 1. 删除不连通的主干块（细胞质块和细胞核）
		for (auto it = Blocks.begin(); it != Blocks.end(); ) {
			int idx = it - Blocks.begin();
			bool isMain = ((int)it->parttype >= 1 && (int)it->parttype < CONNECT_INDEX) || it->organelle == Organelle::nucleus;
			if (isMain && !connected[idx]) {
				it = Blocks.erase(it);
			} else {
				++it;
			}
		}
		// 2. 删除没有相邻主干块的附属块
		for (auto it = Blocks.begin(); it != Blocks.end(); ) {
			bool isMain = ((int)it->parttype >= 1 && (int)it->parttype < CONNECT_INDEX) || it->organelle == Organelle::nucleus;
			if (!isMain) {
				bool hasMainNeighbor = false;
				Vector2 dirs[4] = {{0,-32},{0,32},{-32,0},{32,0}};
				for (auto& d : dirs) {
					Vector2 nPos = it->local + d;
					for (auto& neighbor : Blocks) {
						if (neighbor.local == nPos) {
							int nType = (int)neighbor.parttype;
							if ((nType >= 1 && nType < CONNECT_INDEX) || neighbor.organelle == Organelle::nucleus) {
								hasMainNeighbor = true;
								break;
							}
						}
					}
					if (hasMainNeighbor) break;
				}
				if (!hasMainNeighbor) {
					it = Blocks.erase(it);
				} else {
					++it;
				}
			} else {
				++it;
			}
		}
		int index=0;
		for(auto block=Blocks.begin();block!=Blocks.end();)
		{
			bool is_illegal=false;
			int up=0,left=0,down=0,right=0;
			up=Check_other_blocks({block->local.x,block->local.y-32});
			left=Check_other_blocks({block->local.x-32,block->local.y});
			down=Check_other_blocks({block->local.x,block->local.y+32});
			right=Check_other_blocks({block->local.x+32,block->local.y});
			
			int tot=up+left+down+right;
			if(tot==0 && block->organelle!=Organelle::nucleus)
			{
				is_illegal=true;
			}
			int self_m=static_cast<int>(block->parttype);
			if(self_m>=1 && self_m<CONNECT_INDEX)
			{
				if(tot==0)
				{
					block->parttype=PartType::cy_4;
					block->direction=0.0f;
				}
				if(tot==1)
				{
					block->parttype=PartType::cy_3;
					if(up==1) block->direction=180.0f;
					if(left==1) block->direction=90.0f;
					if(down==1) block->direction=0.0f;
					if(right==1) block->direction=-90.0f;
				}
				if(tot==2)
				{
					if(up==1 && left==1){block->parttype=PartType::cy_2c;block->direction=180.0f;}
					if(left==1 && down==1){block->parttype=PartType::cy_2c;block->direction=90.0f;}
					if(down==1 && right==1){block->parttype=PartType::cy_2c;block->direction=0.0f;}
					if(right==1 && up==1){block->parttype=PartType::cy_2c;block->direction=-90.0f;}
					if(up==1 && down==1){block->parttype=PartType::cy_2p;block->direction=90.0f;}
					if(left==1 && right==1){block->parttype=PartType::cy_2p;block->direction=0.0f;}
				}
				if(tot==3)
				{
					block->parttype=PartType::cy_1;
					if(up==0) block->direction=0.0f;
					if(left==0) block->direction=-90.0f;
					if(down==0) block->direction=180.0f;
					if(right==0) block->direction=90.0f;
				}
				if(tot==4)
				{
					block->parttype=PartType::cy_0;
					block->direction=0.0f;
				}
			}
			if(self_m>=CONNECT_INDEX && self_m<BLOCK_ALLTYPE)
			{
				if(up==1) block->direction=180.0f;
				if(left==1) block->direction=90.0f;
				if(down==1) block->direction=0.0f;
				if(right==1) block->direction=-90.0f;
				if(block->parttype==PartType::mount)
				{
					Flagellum this_flagellum;
					this_flagellum.baseblock_index=index;
					for(int i=1;i<=8;i++)
					{
						Vector2 pos={(float)-i+position.x+block->rocate.x,0};
						this_flagellum.Flagellum_points.emplace_back(pos);
					}
					Flagella.push_back(this_flagellum);
				}
			}
			if(is_illegal)
			{
				block=Blocks.erase(block);
			}
			else
			{
				++block;
			}
			index++;
		}
		found_center();
	}
	void delete_dead(Camera2D& gamecamera,SoundPool& Sound_pool,Sound& broken2)
	{
		for(auto block=Blocks.begin();block!=Blocks.end();)
		{
			if(block->hp<=0.0f)
			{
				Particle_master.createparticle(Particletype::geometricslime,position+block->rocate,Vector2{-300,300},0,color,400,RandomInt(3,5));
				Particle_master.createparticle(Particletype::deadring,position+block->rocate,Vector2{0,0},0,{255,255,255,200},400,1);
				if(block->organelle==Organelle::nucleus) isdead=true;
				float dist=Vector2Distance(position+block->rocate,gamecamera.target)/32.0f;
				Sound_pool.play(broken2,min(1.2f,15.0f/dist));
				Drop_master.createdrop(Droptype::sugermonomer,position+block->rocate,50,0,RandomInt(1,2));
				Drop_master.createdrop(Droptype::sugerdimer,position+block->rocate,50,0,RandomInt(-5,2));
				block=Blocks.erase(block);
				need_to_rebuild=true;
			}
			else
			{
				block++;
			}
		}
	}
	void force(Vector2 actionpoint,Vector2 f)
	{
		velocity.x+=f.x/mass;
		velocity.y+=f.y/mass;
		if(inertia>0.0001f)
		{
			float torque=actionpoint.x*f.y-actionpoint.y*f.x;
			vd+=torque/inertia;
		}
		else
		{
			vd=0.0f;
		}
	}
	Findblockpickresult Find_block_localposition(Vector2 local)
	{
		int index=0;
		for(const auto& block:Blocks)
		{
			if(block.local==local)
			{
				int parttype_id=static_cast<int>(block.parttype);
				if(parttype_id>=1 && parttype_id<CONNECT_INDEX)
				{
					return {true,true,index,block.organelle==Organelle::nucleus};
				}
				else
				{
					return {true,false,index,false};
				}
			}
			index++;
		}
		return {false,false,-1,false};
	}
	void createblock(int cellid,Vector2 local,float mass,float hp,PartType parttype,Organelle organelle)
	{
		Block newBlock(cellid,nextblockID++,local,mass,hp,parttype,organelle);
		Blocks.push_back(newBlock);
	}
};
vector<Cell> Livings;

struct Cellbuilder
{
	int nextID=0;
	int playerID=-1;
	void create(Type type, Vector2 pos, float dir) {//这个函数是deepseek帮忙修改的，有多余空格
		Cell newCell;
		newCell.id = nextID++;
		newCell.position = pos;
		newCell.direction = dir;
		newCell.type = type;
		newCell.nextblockID = 0; // 确保每个新细胞的 blockID 从 0 开始
		
		switch(type) {
		case Type::player:
			playerID = newCell.id;
			newCell.color = {0, 255, 0, 255};
			newCell.createblock(newCell.id, Vector2{-1,0}, 1.5f, 50.0f, PartType::cy_0, Organelle::nucleus);
			newCell.createblock(newCell.id, Vector2{0,0}, 1.1f, 50.0f, PartType::cy_0, Organelle::mitochondrion);
			newCell.createblock(newCell.id, Vector2{1,0}, 1.5f, 50.0f, PartType::cy_0, Organelle::specialized_oral_groove);
			newCell.createblock(newCell.id, Vector2{-2,0}, 0.4f, 30.0f, PartType::mount, Organelle::none);
			newCell.createblock(newCell.id, Vector2{0,-1}, 0.2f, 20.0f, PartType::cilium_left, Organelle::none);
			newCell.createblock(newCell.id, Vector2{0,1}, 0.2f, 20.0f, PartType::cilium_right, Organelle::none);
			break;
			
			case Type::chlorella: {
				newCell.color = {255, 255, 0, 255};
				int sizeclass = RandomInt(1, 3);
				switch(sizeclass) {
				case 1:
					newCell.createblock(newCell.id, Vector2{0,0}, 1.5f, 30.0f, PartType::cy_0, Organelle::nucleus);
					newCell.createblock(newCell.id, Vector2{-1,0}, 1.0f, 30.0f, PartType::cy_0, Organelle::none);
					newCell.createblock(newCell.id, Vector2{1,0}, 1.0f, 30.0f, PartType::cy_0, Organelle::none);
					newCell.createblock(newCell.id, Vector2{0,-1}, 1.0f, 30.0f, PartType::cy_0, Organelle::none);
					newCell.createblock(newCell.id, Vector2{0,1}, 1.0f, 30.0f, PartType::cy_0, Organelle::none);
					break;
				case 2:
					newCell.createblock(newCell.id, Vector2{0,0}, 2.0f, 50.0f, PartType::cy_0, Organelle::nucleus);
					newCell.createblock(newCell.id, Vector2{-1,0}, 1.2f, 40.0f, PartType::cy_0, Organelle::none);
					newCell.createblock(newCell.id, Vector2{1,0}, 1.2f, 40.0f, PartType::cy_0, Organelle::none);
					newCell.createblock(newCell.id, Vector2{0,-1}, 1.2f, 40.0f, PartType::cy_0, Organelle::none);
					newCell.createblock(newCell.id, Vector2{0,1}, 1.2f, 40.0f, PartType::cy_0, Organelle::none);
					newCell.createblock(newCell.id, Vector2{-1,-1}, 0.8f, 30.0f, PartType::cy_0, Organelle::none);
					newCell.createblock(newCell.id, Vector2{1,-1}, 0.8f, 30.0f, PartType::cy_0, Organelle::none);
					newCell.createblock(newCell.id, Vector2{-1,1}, 0.8f, 30.0f, PartType::cy_0, Organelle::none);
					newCell.createblock(newCell.id, Vector2{1,1}, 0.8f, 30.0f, PartType::cy_0, Organelle::none);
					newCell.createblock(newCell.id, Vector2{-2,0}, 0.6f, 30.0f, PartType::cy_0, Organelle::none);
					newCell.createblock(newCell.id, Vector2{2,0}, 0.6f, 30.0f, PartType::cy_0, Organelle::none);
					newCell.createblock(newCell.id, Vector2{0,-2}, 0.6f, 30.0f, PartType::cy_0, Organelle::none);
					newCell.createblock(newCell.id, Vector2{0,2}, 0.6f, 30.0f, PartType::cy_0, Organelle::none);
					break;
				case 3:
					newCell.createblock(newCell.id, Vector2{0,0}, 2.5f, 80.0f, PartType::cy_0, Organelle::nucleus);
					// 十字
					newCell.createblock(newCell.id, Vector2{-1,0}, 1.5f, 50.0f, PartType::cy_0, Organelle::none);
					newCell.createblock(newCell.id, Vector2{1,0}, 1.5f, 50.0f, PartType::cy_0, Organelle::none);
					newCell.createblock(newCell.id, Vector2{0,-1}, 1.5f, 50.0f, PartType::cy_0, Organelle::none);
					newCell.createblock(newCell.id, Vector2{0,1}, 1.5f, 50.0f, PartType::cy_0, Organelle::none);
					// 四角
					newCell.createblock(newCell.id, Vector2{-1,-1}, 1.0f, 40.0f, PartType::cy_0, Organelle::none);
					newCell.createblock(newCell.id, Vector2{1,-1}, 1.0f, 40.0f, PartType::cy_0, Organelle::none);
					newCell.createblock(newCell.id, Vector2{-1,1}, 1.0f, 40.0f, PartType::cy_0, Organelle::none);
					newCell.createblock(newCell.id, Vector2{1,1}, 1.0f, 40.0f, PartType::cy_0, Organelle::none);
					// 外圈十字
					newCell.createblock(newCell.id, Vector2{-2,0}, 0.6f, 30.0f, PartType::cy_0, Organelle::none);
					newCell.createblock(newCell.id, Vector2{2,0}, 0.6f, 30.0f, PartType::cy_0, Organelle::none);
					newCell.createblock(newCell.id, Vector2{0,-2}, 0.6f, 30.0f, PartType::cy_0, Organelle::none);
					newCell.createblock(newCell.id, Vector2{0,2}, 0.6f, 30.0f, PartType::cy_0, Organelle::none);
					// 扩张
					newCell.createblock(newCell.id, Vector2{0,3}, 0.6f, 30.0f, PartType::cy_0, Organelle::none);
					newCell.createblock(newCell.id, Vector2{1,2}, 0.6f, 30.0f, PartType::cy_0, Organelle::none);
					newCell.createblock(newCell.id, Vector2{2,1}, 0.6f, 30.0f, PartType::cy_0, Organelle::none);
					newCell.createblock(newCell.id, Vector2{3,0}, 0.6f, 30.0f, PartType::cy_0, Organelle::none);
					newCell.createblock(newCell.id, Vector2{2,-1}, 0.6f, 30.0f, PartType::cy_0, Organelle::none);
					newCell.createblock(newCell.id, Vector2{1,-2}, 0.6f, 30.0f, PartType::cy_0, Organelle::none);
					newCell.createblock(newCell.id, Vector2{0,-3}, 0.6f, 30.0f, PartType::cy_0, Organelle::none);
					newCell.createblock(newCell.id, Vector2{-1,-2}, 0.6f, 30.0f, PartType::cy_0, Organelle::none);
					newCell.createblock(newCell.id, Vector2{-2,-1}, 0.6f, 30.0f, PartType::cy_0, Organelle::none);
					newCell.createblock(newCell.id, Vector2{-3,0}, 0.6f, 30.0f, PartType::cy_0, Organelle::none);
					newCell.createblock(newCell.id, Vector2{-2,1}, 0.6f, 30.0f, PartType::cy_0, Organelle::none);
					newCell.createblock(newCell.id, Vector2{-1,2}, 0.6f, 30.0f, PartType::cy_0, Organelle::none);
					break;
				default:
					break;
				}
				
				// 随机生成刺细胞
				float spikechance = 0.3f;
				Vector2 dirs[4] = {{0,-32},{0,32},{-32,0},{32,0}};
				for (const auto& block : newCell.Blocks) {
					int type = (int)block.parttype;
					if (!(type >= 1 && type < CONNECT_INDEX)) continue;
					for (const auto& d : dirs) {
						Vector2 checkpos = block.local + d;
						bool occupied = false;
						for (const auto& existing : newCell.Blocks) {
							if (existing.local == checkpos) {
								occupied = true;
								break;
							}
						}
						if (!occupied && RandomFloat(0.0f, 1.0f) < spikechance) {
							Vector2 gridpos = {checkpos.x / 32, checkpos.y / 32};
							newCell.createblock(newCell.id, gridpos, 0.8f, 20.0f, PartType::spike, Organelle::none);
						}
					}
				}
				break;
			}
		default:
			return;
			break;
		}
		
		Livings.push_back(newCell);
	}
};

struct Waveform
{
	Vector2 baseposition={145,240};
	float position=0.0f,velocity=0.0f;
	float maxposition=240.0f;
	Rectangle source;
};
vector<Waveform> waves;
struct UI
{
	Vector2 position={0,0},moveto;
	float lerp=0.15f;
	Vector2 position2={0,0},moveto2;
	float lerp2=0.15f;
	bool fold,left,active=false;
	float timer=0.0f,timer2=0.0f,timer3=0.0f,timer4=0.0f;
	Rectangle source;
	bool warning=false;
	bool dying=false;
	bool dead=false;
	void update(int type,float deltaT)
	{
		position+=(moveto-position)*lerp;
		if(fold)
		{
			position2+=(Vector2Negate(position2))*lerp2;
		}
		else
		{
			position2+=(moveto2-position2)*lerp2;
		}
		switch(type)
		{
		case 1:
			break;
		case 2:
			timer+=8.0f*deltaT;
			if(timer>=8.0f) timer-=8.0f;
			if(playerID!=-1 && playernucleusindex!=-1) timer2+=20.0f/Livings[playerID].Blocks[playernucleusindex].hp*deltaT+0.001f;
			if(timer2>=1.0f)
			{
				timer2-=1.0f;
				Waveform newWave;
				newWave.velocity=100.0f;
				newWave.source={284,78,18,26};
				waves.push_back(newWave);
			}
			if(warning)
			{
				if(timer3>0.0f) timer3-=deltaT;
				else warning=false;
			}
			else
			{
				timer3=0.2f;
			}
			for(auto wave=waves.begin();wave!=waves.end();)
			{
				wave->position+=wave->velocity*deltaT;
				if(wave->position>wave->maxposition)
				{
					wave=waves.erase(wave);
				}
				else
				{
					wave++;
				}
			}
			break;
		case 3:
			timer+=8.0f*deltaT;
			if(timer>=19.0f) timer-=19.0f;
			break;
		case 4://creater界面
			timer2+=deltaT;
			if(timer<1.0f){timer+=0.6f*deltaT;break;}
			timer=1.0f;
			if(timer2<4.0f) break;
			if(timer3<1.0f){timer3+=deltaT;break;}
			timer3=1.0f;
			if(timer4<2.0f){timer4+=deltaT;break;}
			interface=GameState::GAMING;
			break;
		default:
			break;
		}
	}
	void draw(Texture2D texture,int type,Font font,string text)
	{
		const float scale=1.7f;
		switch(type)
		{
		case 1:
			DrawTexturePro(texture,source,{position.x,position.y,284*scale,56*scale},{0,0},0.0f,WHITE);
			DrawTexturePro(texture,{0,56,284,1},{position.x,position.y+56*scale,284*scale,position.y+position2.y+4*scale},{0,0},0.0f,WHITE);
			DrawTexturePro(texture,{0,56,284,13},{position.x,position.y+position2.y+112,284*scale,26*scale},{0,0},0.0f,WHITE);
			if(left)
			{
				DrawTextEx(font,text.c_str(),{position.x+50,position.y+28},48,0,WHITE);
			}
			else
			{
				DrawTextEx(font,text.c_str(),{position.x+240,position.y+28},48,0,WHITE);
			} 
			break;
		case 2:
		{
			DrawTexturePro(texture,source,{position.x,position.y,source.width*scale,source.height*scale},{0,0},0.0f,WHITE);
			DrawTextEx(font,text.c_str(),{position.x+570,position.y+230},48,0,WHITE);
			Color warningcolor;
			if(warning) warningcolor={255,0,0,255};
			else warningcolor={255,255,255,255};
			if(playerID!=-1)
			{
				dead=false;
				DrawTexturePro(texture,{floor(timer)*64.0f,292,64,64},{position.x+17,position.y+160,64*scale,64*scale},{0,0},0.0f,warningcolor);
			}
			else dead=true;
			for(auto& wave:waves)
			{
				DrawTexturePro(texture,wave.source,{position.x+wave.baseposition.x+wave.position,position.y+wave.baseposition.y,wave.source.width*scale,wave.source.height*scale},{0,0},0.0f,WHITE);
			}
			float texthp;
			if(playerID==-1) texthp=0.0f;
			else texthp=Livings[playerID].Blocks[playernucleusindex].hp;
			DrawTextEx(font,TextFormat("%.1f",texthp),{position.x+157,position.y+184},36,0,RED);
			if(texthp<=5.0f) dying=true;
			else dying=false;
			if(dead) warningcolor=RED;
			else if(dying)
			{
				if((int)timer%2==0)
				{
					warningcolor=RED;
					DrawTextEx(font,"濒死warning",{position.x+570,position.y+150},72,0,RED);
				}
				else warningcolor={0,0,200,255};
			}
			else warningcolor={0,0,200,255};
			DrawTexturePro(texture,{416,0,36,36},{position.x+22,position.y-9,48*scale,48*scale},{0,0},0.0f,warningcolor);
			DrawTexturePro(texture,{452,0,66,16},{position.x+415,position.y+190,88*scale,21*scale},{0,0},0.0f,warningcolor);
			break;
		}
		case 3:
		{
			DrawTexturePro(texture,source,{position.x,position.y,source.width*scale,source.height*scale},{0,0},0.0f,WHITE);
			DrawTextEx(font,text.c_str(),{position.x-170,position.y+230},48,0,WHITE);
			float textenergy;
			Color warning;
			if(playerID!=-1)
			{
				float t=(Livings[playerID].energy/Livings[playerID].maxenergy);
				DrawRectanglePro({position.x+545,position.y+280,10.0f*scale,t*200.0f},{0,0},180.0f,{0,255,255,255});
				textenergy=Livings[playerID].energy;
				int row=(int)timer/3;
				int col=(int)timer%3;
				if((int)(timer*2)%2==0 && textenergy<20.0f) warning=RED;
				else warning=WHITE;
				DrawTexturePro(texture,{(float)col*96,(float)row*48+356,96,48},{position.x+310,position.y+190,120*scale,60*scale},{0,0},0.0f,warning);
				dead=false;
				if(Livings[playerID].energy<=0.001f) dying=true;
				else dying=false;
			}
			else
			{
				dead=true;
				textenergy=0.0f;
			}
			DrawTextEx(font,TextFormat("%.1f",textenergy),{position.x+435,position.y+143},36,0,{0,255,255,255});
			if(dead) warning=RED;
			else if(dying)
			{
				if((int)timer%2==0)
				{
					warning=RED;
					DrawTextEx(font,"warning濒死",{position.x-370,position.y+150},72,0,RED);
				}
				else warning={0,0,200,255};
			}
			else warning={0,0,200,255};
			DrawTexturePro(texture,{517,0,36,36},{position.x+452,position.y-9,48*scale,48*scale},{0,0},0.0f,warning);
			DrawTexturePro(texture,{554,0,66,16},{position.x,position.y+190,88*scale,21*scale},{0,0},0.0f,warning);
			break;
		}
		case 4:
		{
			float alpha=timer*(1.0f-timer3);
			int f=(int)(timer2*8)%3;
			float scale=1.5f;
			DrawTexturePro(texture,{f*64.0f,0,64,64},{SCREEN_WIDTH/2.0f,SCREEN_HEIGHT/2.0f,64*scale,64*scale},{32*scale,32*scale},0.0f,{255,255,255,(unsigned char)(alpha*255)});
			break;
		}
		default:
			break;
		}
		
	}
};

struct Part
{
	int id;
	string name;
	Rectangle source;
	float mass;
	float hp;
	PartType parttype;
	Organelle organelle;
	float retimer;
	float timer;
	Vector2 position;
	Rectangle dest,dest2;
	bool pressed=false;
	bool is_hovering;
	void update(float deltaT,Vector2 pos,float scale,float spacing,float width,UI &ui)
	{
		if(timer>-0.1)
		{
			timer-=deltaT;
		}
		int raw=width/(44*scale+spacing);
		int grid_x=(id%raw);
		int grid_y=(id/raw);
		Vector2 grid={(float)grid_x,(float)grid_y};
		position=pos+grid*44*scale+grid*spacing;
		dest={position.x,position.y,44*scale,44*scale};
		if(!pressed)
		{
			dest2.x=dest.x;
			dest2.y=dest.y;
			dest2.width=32*scale;
			dest2.height=32*scale;
		}
		else
		{
			dest2={GetMouseX()-22*scale,GetMouseY()-22*scale,32*scale,32*scale};
		}
	}
	void draw(Texture2D texture_back,Texture2D texture,float scale)
	{
		if(timer>0) return;
		Rectangle back_source;
		if(timer>-0.1)
		{
			back_source={284,0,44,44};
		}
		else
		{
			if(is_hovering) back_source={372,0,44,44};
			else back_source={328,0,44,44};
		}
		DrawTexturePro(texture_back,back_source,dest,{0,0},0.0f,WHITE);
		if(timer>-0.1) return;
		DrawTexturePro(texture,source,dest2,{-6*scale,-6*scale},0.0f,WHITE);
	}
};


float point_to_point(float x1,float y1,float x2,float y2)
{
	return RAD2DEG*atan2f(y2-y1,x2-x1);
}

int mod(int a,int b)
{
	return (a%b+b)%b;
}

void UpdateGaming(float deltaT,GameState& interface,Camera2D& gamecamera,Camera2D& assemcamera,Vector2& mouseworldposition,bool& zooming,vector<Part>&Partlibrary,UI& ui1,Music& bgm,UI& ui2,SoundPool& Sound_pool,Sound& shoot1,Sound& shoot2,Sound& hit,Sound& broken1,Sound& broken2,Sound& dong,UI& ui3,Sound& di)
{
	mouseworldposition=GetScreenToWorld2D(GetMousePosition(),gamecamera);
	UpdateMusicStream(bgm);
	if(!IsMusicStreamPlaying(bgm)) PlayMusicStream(bgm);
	shake.x*=RandomFloat(-0.99f,-0.5f);
	shake.y*=RandomFloat(-0.99f,-0.5f);
	for(int i=0;i<MAX_BULLETS;i++)//遍历子弹
	{
		if(Bullet_pool.Bullets[i].is_active)
		{
			Bullet_pool.Bullets[i].lifetime-=deltaT;
			if(Bullet_pool.Bullets[i].lifetime<0)
			{
				Bullet_pool.Bullets[i].is_active=false;
				Bullet_pool.count--;
				continue;
			}
			for(int j=0;j<Bullet_pool.substeps;j++)
			{
				Bullet_pool.Bullets[i].position.x+=Bullet_pool.Bullets[i].velocity.x/Bullet_pool.substeps;
				Bullet_pool.Bullets[i].position.y+=Bullet_pool.Bullets[i].velocity.y/Bullet_pool.substeps;
				for(auto& enemy:Livings)
				{
					if(enemy.id==Bullet_pool.Bullets[i].belongto_living_index) continue;
					if(fabs(enemy.position.x-Bullet_pool.Bullets[i].position.x)>500.0f &&
					   fabs(enemy.position.y-Bullet_pool.Bullets[i].position.y)>500.0f) continue;
					for(auto& block:enemy.Blocks)
					{
						Vector2 xydist=Bullet_pool.Bullets[i].position-(enemy.position+block.rocate);
						if(!(fabs(xydist.x)<25.0f && fabs(xydist.y)<25.0f)) continue;
						float rad=DEG2RAD*-(enemy.direction+block.direction);
						Vector2 rocateback;
						rocateback.x=cosf(rad)*xydist.x-sinf(rad)*xydist.y;
						rocateback.y=sinf(rad)*xydist.x+cosf(rad)*xydist.y;
						if(!(fabs(rocateback.x)<16.0f && fabs(rocateback.y)<16.0f)) continue;
						block.hp-=Bullet_pool.Bullets[i].damage;
						enemy.force({block.rocate.x,block.rocate.y},Vector2Normalize(Bullet_pool.Bullets[i].velocity)*30.0f);
						Particle_master.createparticle(Particletype::hit,enemy.position+block.rocate,Bullet_pool.Bullets[i].velocity,Bullet_pool.Bullets[i].direction,Color{0,0,0,0},0,3);
						block.hittimer=0.1f;
						if(block.organelle==Organelle::nucleus && block.hp<=0.0f) enemy.isdead=true;
						float dist=Vector2Distance(Bullet_pool.Bullets[i].position,gamecamera.target)/32.0f;
						float shakeclass=20.0f*min(1.0f,20.0f/dist+0.01f);
						shake.x=RandomFloat(-shakeclass,shakeclass);
						shake.y=RandomFloat(-shakeclass,shakeclass);
						Sound_pool.play(hit,min(1.0f,10.0f/dist));
						if(enemy.type==Type::player)
						{
							Waveform newWave;
							newWave.source={302,78,20,26};
							newWave.velocity=180.0f;
							waves.push_back(newWave);
							if(block.organelle==Organelle::nucleus)
							{
								ui2.warning=true;
							}
						}
						for(auto& other:Livings[Bullet_pool.Bullets[i].belongto_living_index].Blocks)
						{
							if(other.organelle==Organelle::mitochondrion && !other.overload)
							{
								Vector2 shooterlocal={0,0};
								bool found_shooter=false;
								for(auto& shooter:Livings[Bullet_pool.Bullets[i].belongto_living_index].Blocks)
								{
									if(shooter.blockID==Bullet_pool.Bullets[i].belongto_block_index)
									{
										found_shooter=true;
										shooterlocal=shooter.local;
										break;
									}
								}
								if(found_shooter)
								{
									float d=Vector2Distance(shooterlocal,other.local);
									if(d<40.0f)
									{
										other.charge+=2.0f;
										if(other.charge>=other.maxcharge)
										{
											other.charge=other.maxcharge;
											other.overload=true;
											other.overloadtimer=3.0f;
											Particle_master.createparticle(Particletype::overloadring,Livings[Bullet_pool.Bullets[i].belongto_living_index].position+other.rocate,{0,0},0,YELLOW,200,1);
											Sound_pool.play(dong,min(0.8f,10.0f/dist));
										}
									}
								}
							}
						}
						Bullet_pool.Bullets[i].is_active=false;
						if(!Bullet_pool.Bullets[i].is_active) break;
					}
					if(!Bullet_pool.Bullets[i].is_active) break;
				}
				if(!Bullet_pool.Bullets[i].is_active) break;
			}
		}
	}
	for(auto& it:Livings)//遍历生物
	{
		it.update(deltaT,gamecamera,Sound_pool,shoot1,shoot2,di);
		it.delete_dead(gamecamera,Sound_pool,broken2);
		if(it.need_to_rebuild)
		{
			it.need_to_rebuild=false;
			it.rebulildcell();
		}
		switch(it.type)
		{
		case Type::player:
		{
			for(auto& block:it.Blocks)
			{
				switch(block.organelle)
				{
				case Organelle::specialized_oral_groove:
					block.organelle_direction=point_to_point(it.position.x+block.rocate.x,it.position.y+block.rocate.y,mouseworldposition.x,mouseworldposition.y);
					if(IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !zooming) block.state=Blockstate::active;
					else block.state=Blockstate::none;
					break;
				case Organelle::nucleus:
					if(IsKeyPressed(KEY_E) && !it.energydying) 
					{
						zooming=true;
					}
					gamecamera.target.x+=(it.position.x+block.rocate.x-gamecamera.target.x)*0.15;
					gamecamera.target.y+=(it.position.y+block.rocate.y-gamecamera.target.y)*0.15;
					if(zooming)
					{
						ui2.moveto={-400.0f,SCREEN_HEIGHT};
						ui3.moveto={SCREEN_WIDTH-180.0f,SCREEN_HEIGHT};
						gamecamera.zoom+=gamecamera.zoom*gamecamera.zoom*deltaT;
						if(gamecamera.zoom>=400.0f)
						{
							zooming=false;
							assemcamera.zoom=0.01;
							ui1.moveto={30,50};
							ui1.moveto2={0,400};
							ui1.position2={0,0};
							ui2.position={-400.0f,SCREEN_HEIGHT};
							ui2.moveto={20.0f,SCREEN_HEIGHT-300};
							ui3.position={SCREEN_WIDTH-180.0f,SCREEN_HEIGHT};
							ui3.moveto={SCREEN_WIDTH-560.0f,SCREEN_HEIGHT-300};
							for(auto& it:Partlibrary)
							{
								it.timer=it.retimer;
							}
							interface=GameState::ASSEMBLING;
						}
					}
					else
					{
						if(it.energydying) gamecamera.zoom+=(2.0f-gamecamera.zoom)*0.06;
						else if(it.energy<20.0f) gamecamera.zoom+=(1.5f-gamecamera.zoom)*0.07;
						else gamecamera.zoom+=(1.0f-gamecamera.zoom)*0.1;
						gamecamera.offset.x=(SCREEN_WIDTH/2)+shake.x;
						gamecamera.offset.y=(SCREEN_HEIGHT/2)+shake.y;
					}
					break;
				case Organelle::none:
					switch(block.parttype)
					{
					case PartType::mount:
						if(IsKeyDown(KEY_W) && !zooming) block.state=Blockstate::active;
						else block.state=Blockstate::none;
						break;
					case PartType::cilium_left:
						if(IsKeyDown(KEY_D) && !zooming) block.state=Blockstate::active;
						else block.state=Blockstate::none;
						break;
					case PartType::cilium_right:
						if(IsKeyDown(KEY_A) && !zooming) block.state=Blockstate::active;
						else block.state=Blockstate::none;
						break;
					default:
						break;
					}
				default:
					break;
				}
			}
			break;
		}
		case Type::chlorella:
		{
			for(auto& block:it.Blocks)
			{
				if(block.parttype==PartType::spike)
				{
					block.state=Blockstate::active;
				}
			}
			break;
		}
		default:
			break;
		}
	}
	for(auto it=Livings.begin();it!=Livings.end();)
	{
		if(it->isdead || it->Blocks.empty())
		{
			if(it->id==playerID)
			{
				playerID=-1;
			}
			Particle_master.createparticle(Particletype::geometricslime,it->position+it->center,Vector2{-1200,1200},0,it->color,1200,RandomInt(8,12));
			Particle_master.createparticle(Particletype::deadring,it->position+it->center,Vector2{0,0},0,{255,255,255,200},1500,1);
			float dist=Vector2Distance(it->position+it->center,gamecamera.target)/32.0f;
			float shakeclass=150.0f*min(1.0f,30.0f/dist);
			shake.x=RandomFloat(-shakeclass,shakeclass);
			shake.y=RandomFloat(-shakeclass,shakeclass);
			Sound_pool.play(broken1,min(1.5f,25.0f/dist));
			Drop_master.createdrop(Droptype::sugermonomer,it->position+it->center,200,0,RandomInt(3,5));
			Drop_master.createdrop(Droptype::sugerdimer,it->position+it->center,200,0,RandomInt(1,3));
			Drop_master.createdrop(Droptype::sugerpolymer,it->position+it->center,200,0,RandomInt(0,2));
			it=Livings.erase(it);
		}
		else
		{
			++it;
		}
	}
	for(auto it=Particle_master.Particles.begin();it!=Particle_master.Particles.end();)
	{
		if(it->lifetime<0.0f)
		{
			it=Particle_master.Particles.erase(it);
		}
		else
		{
			++it;
		}
	}
	for(auto& it:Particle_master.Particles)
	{
		it.update(deltaT);
	}
	Sound_pool.update();
	ui2.update(2,deltaT);
	ui3.update(3,deltaT);
	Drop_master.update(deltaT);
}

void UpdateAssembling(float deltaT,GameState& interface,Camera2D& gamecamera,Camera2D& assemcamera,Cellbuilder builder,Vector2& mouseworldposition,bool& zooming,vector<Part>&Partlibrary,UI& ui1,Vector2& UI1_grid)
{
	mouseworldposition=GetScreenToWorld2D(GetMousePosition(),assemcamera);
	assemcamera.target=Vector2Scale(mouseworldposition,0.2f);
	ui1.update(1,deltaT);
	UI1_grid.x=(int)roundf(((mouseworldposition.x)/32.0f));
	UI1_grid.y=(int)roundf(((mouseworldposition.y)/32.0f));
	if(!zooming)
	{
		if(ui1.position.x>0.0f) ui1.fold=false;
		if(ui1.active)
		{
			assemcamera.zoom+=(3.5f-assemcamera.zoom)*0.1;
			if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
			{
				Part this_part;
				for(const auto& part:Partlibrary)
				{
					if(part.pressed)
					{
						this_part=part;
						break;
					}
				}
				Findblockpickresult result;
				result=Livings[builder.playerID].Find_block_localposition({(UI1_grid.x)*32,(UI1_grid.y)*32});
				if(!result.is_nucleus){
					if(this_part.organelle==Organelle::none)
					{
						if(this_part.parttype==PartType::none)
						{
							result=Livings[builder.playerID].Find_block_localposition(UI1_grid*32);
							if(result.is_part)
							{
								Livings[builder.playerID].Blocks.erase(Livings[builder.playerID].Blocks.begin()+result.block_index);
								Livings[builder.playerID].rebulildcell();
							}
						}
						else
						{
							int sum=0;
							result=Livings[builder.playerID].Find_block_localposition({(UI1_grid.x+1)*32,(UI1_grid.y+0)*32});
							sum+=result.is_cytosol;
							result=Livings[builder.playerID].Find_block_localposition({(UI1_grid.x-1)*32,(UI1_grid.y+0)*32});
							sum+=result.is_cytosol;
							result=Livings[builder.playerID].Find_block_localposition({(UI1_grid.x+0)*32,(UI1_grid.y+1)*32});
							sum+=result.is_cytosol;
							result=Livings[builder.playerID].Find_block_localposition({(UI1_grid.x+0)*32,(UI1_grid.y-1)*32});
							sum+=result.is_cytosol;
							if(sum>0)
							{
								result=Livings[builder.playerID].Find_block_localposition({(UI1_grid.x)*32,(UI1_grid.y)*32});
								if(result.is_part)
								{
									Livings[builder.playerID].Blocks[result.block_index].parttype=this_part.parttype;
									Livings[builder.playerID].Blocks[result.block_index].organelle=Organelle::none;
								}
								else
								{
									Block block(Livings[builder.playerID].id,Livings[builder.playerID].nextblockID++,UI1_grid,this_part.mass,this_part.hp,this_part.parttype,this_part.organelle);
									Livings[builder.playerID].Blocks.push_back(block);
								}
								Livings[builder.playerID].rebulildcell();
							}
						}
					}
					else
					{
						result=Livings[builder.playerID].Find_block_localposition({UI1_grid.x*32,UI1_grid.y*32});
						if(result.is_cytosol)
						{
							Livings[builder.playerID].Blocks[result.block_index].organelle=this_part.organelle;
							Livings[builder.playerID].Blocks[result.block_index].mass=1.0f+this_part.mass;
							Livings[builder.playerID].rebulildcell();
						}
					}
				}
			}
		}
		else
		{
			assemcamera.zoom+=(1.5f-assemcamera.zoom)*0.06;
		}
	}
	else
	{
		ui1.fold=true;
		ui1.moveto2.y=0;
		assemcamera.zoom-=4.0f*deltaT;
		if(assemcamera.zoom<1.0f) ui1.moveto.x=-650;
		if(assemcamera.zoom<0.05f)
		{
			gamecamera.zoom=400.0f;
			zooming=false;
			interface=GameState::GAMING;
		}
	}
	if(IsKeyPressed(KEY_E)) zooming=true;
	if(ui1.position2.y>300)
	{
		for(auto& it:Partlibrary)
		{
			float scale=1.6f;
			it.update(deltaT,{ui1.position.x+40,ui1.position.y+140},scale,10.0f,284*1.7-80,ui1);
			bool is_hovering=CheckCollisionPointRec(GetMousePosition(),it.dest);
			if(!it.pressed)
			{
				it.dest2.x=it.dest.x;
				it.dest2.y=it.dest.y;
				it.dest2.width=32*scale;
				it.dest2.height=32*scale;
				if(is_hovering && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
				{
					for(auto& this_part:Partlibrary)
					{
						this_part.pressed=false;
					}
					it.pressed=true;
					ui1.active=true;
				}
			}
			else
			{
				it.dest2={GetMouseX()-22*scale,GetMouseY()-22*scale,32*scale,32*scale};
				if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
				{
					it.pressed=false;
					ui1.active=false;
				}
			}
		}
	}
}

void DrawGaming(Texture2D texture_all_parts,Texture texture_ui,Texture2D background,Camera2D& gamecamera,Cellbuilder& builder,UI& ui2,Font& font,UI& ui3)
{
	DrawTexturePro(background,{0,0,1600,900},{0,0,SCREEN_WIDTH,SCREEN_HEIGHT},{0,0},0.0f,WHITE);
	BeginMode2D(gamecamera);
	if(gamecamera.zoom>=0.05f)
	{
		float halfW=(SCREEN_WIDTH/2.0f)/gamecamera.zoom;//画网格
		float halfH=(SCREEN_HEIGHT/2.0f)/gamecamera.zoom;
		float left=gamecamera.target.x-halfW;
		float right=gamecamera.target.x+halfW;
		float top=gamecamera.target.y-halfH;
		float bottom=gamecamera.target.y+halfH;
		float gridSize=100.0f;
		float startX=floorf(left/gridSize)*gridSize;
		float startY=floorf(top/gridSize)*gridSize;
		for(float x=startX;x<=right;x+=gridSize)
		{
			DrawLineEx({x,top},{x,bottom},5,{255,255,255,15});
		}
		for(float y=startY;y<=bottom;y+=gridSize)
		{
			DrawLineEx({left,y},{right,y},5,{255,255,255,15});
		}
	}
	for(auto& it:Livings)
	{
		for(auto& block:it.Blocks)
		{
			block.Drawblock(texture_all_parts,it.type,it.position+block.rocate,it.direction,block.direction,block.organelle_direction);
		}
		for(const auto& this_flagellum:it.Flagella)
		{
			for(const auto& this_point:this_flagellum.Flagellum_points)
			{
				DrawTexturePro(texture_all_parts,{32,64,32,32},{this_point.position.x,this_point.position.y,32,32},{16,16},0.0f,WHITE);
			}
		}
	}
	for(int i=0;i<MAX_BULLETS;i++)
	{
		
		if(Bullet_pool.Bullets[i].is_active)
		{
			Rectangle source;
			Rectangle dest={Bullet_pool.Bullets[i].position.x,Bullet_pool.Bullets[i].position.y,0,32};
			Rectangle lightdest={dest.x,dest.y,256,256};
			Vector2 origin={0,0};
			switch(Bullet_pool.Bullets[i].type)
			{
			case Bullettype::foodvacuole:
				source={32,96,128,32};
				dest.width=128;
				origin={96,16};
				break;
			case Bullettype::spike:
				source={160,96,96,32};
				dest.width=96;
				origin={64,16};
				break;
			default:
				source={0,96,32,32};
				dest.width=32;
				break;
			};
			DrawTexturePro(texture_all_parts,{0,256,96,96},lightdest,{128,128},Bullet_pool.Bullets[i].direction,Bullet_pool.Bullets[i].lightcolor);
			DrawTexturePro(texture_all_parts,source,dest,origin,Bullet_pool.Bullets[i].direction,WHITE);
		}
	}
	Drop_master.draw();
	for(auto& it:Particle_master.Particles)
	{
		it.draw();
	}
	EndMode2D();
	ui2.draw(texture_ui,2,font,"健康指示");
	ui3.draw(texture_ui,3,font,"能量指示");
}

void DrawAssembling(Texture2D texture_all_parts,Texture2D texture_ui,Camera2D assemcamera,Cellbuilder builder,UI ui1,Vector2 UI1_grid,Font font,vector<Part>&Partlibrary)
{
	ClearBackground(BLACK);
	ui1.draw(texture_ui,1,font,"零件库");
	if(ui1.position2.y>300)
	{
		for(auto& it:Partlibrary)
		{
			it.draw(texture_ui,texture_all_parts,1.6f);
		}
	}
	BeginMode2D(assemcamera);
	for(auto& block:Livings[builder.playerID].Blocks)
	{
		block.Drawblock(texture_all_parts,Type::player,block.local,0.0f,block.direction,0.0f);
	}
	DrawCircleV(Livings[builder.playerID].center,3.0f,RED);
	if(ui1.active)
	{
		DrawTexturePro(texture_ui,{284,44,34,34},{UI1_grid.x*32.0f,UI1_grid.y*32.0f,34,34},{17,17},0.0f,WHITE);
	}
	EndMode2D();
}

void UpdateCreater(float deltaT,UI& ui4)
{
	ui4.update(4,deltaT);
}

void DrawCreater(Texture2D texture,UI& ui4,Font& font)
{
	ClearBackground(BLACK);
	ui4.draw(texture,4,font,"");
}

void UpdateMenu()
{
	
}

void DrawMenu()
{
	
}

int main()
{	
	InitWindow(SCREEN_WIDTH,SCREEN_HEIGHT,"MadCells");
	InitAudioDevice();
	SetTargetFPS(60);
	SetExitKey(KEY_NULL);
	
	bool zooming=false;
	
	Vector2 mouseworldposition;
	
	Camera2D gamecamera={0};
	gamecamera.target={0,0};
	gamecamera.offset={SCREEN_WIDTH/2,SCREEN_HEIGHT/2};
	gamecamera.rotation=0.0f;
	gamecamera.zoom=1.0f;
	Camera2D assemcamera={0};
	assemcamera.target={0,0};
	assemcamera.offset={SCREEN_WIDTH/2,SCREEN_HEIGHT/2};
	assemcamera.rotation=0.0f;
	assemcamera.zoom=1.0f;
	
	Texture2D background=LoadTexture("asset/background.png");
	Texture2D texture_all_parts=LoadTexture("asset/parts.png");
	Texture2D texture_ui=LoadTexture("asset/ui.png");
	Texture2D texture_Dyx=LoadTexture("asset/Dyx.png");
	Sound shoot1=LoadSound("asset/sound/shoot1.wav");
	Sound shoot2=LoadSound("asset/sound/shoot2.wav");
	Sound hit=LoadSound("asset/sound/hit.wav");
	Sound broken1=LoadSound("asset/sound/broken1.wav");
	Sound broken2=LoadSound("asset/sound/broken2.wav");
	Sound dong=LoadSound("asset/sound/dong.wav");
	Sound di=LoadSound("asset/sound/di.wav");
	Music bgm=LoadMusicStream("asset/sound/face_down.mp3");
	SetMusicVolume(bgm,0.8);
	SoundPool Sound_pool;
	Sound_pool.init(shoot1,5);
	Sound_pool.init(shoot2,7);
	Sound_pool.init(hit,7);
	Sound_pool.init(broken1,3);
	Sound_pool.init(broken2,5);
	Sound_pool.init(dong,5);
	Sound_pool.init(di,2);
	
	Cellbuilder builder;
	builder.create(Type::player,Vector2{0,0},0.0f);
	for(int i=0;i<10;i++)
	{
		builder.create(Type::chlorella,Vector2{RandomFloat(-800.0f,800.0f),0+RandomFloat(-500.0f,500.0f)},0.0f);
	}
	
	for(int i=0;i<MAX_BULLETS;i++)
	{
		Bullet_pool.Bullets[i].is_active=false;
	}
	
	UI ui1,ui2,ui3,ui4;
	ui1.fold=true;
	ui1.position={-400.0f,50.0f};
	ui1.left=true;
	ui1.source={0,0,284,56};
	Vector2 UI1_grid;//UI1放置时的网格坐标
	ui2.position={-400.0f,SCREEN_HEIGHT};
	ui2.moveto={20.0f,SCREEN_HEIGHT-300.0f};
	ui2.source={0,126,330,166};
	ui2.lerp=0.05f;
	ui3.position={SCREEN_WIDTH-160.0f,SCREEN_HEIGHT};
	ui3.moveto={SCREEN_WIDTH-580.0f,SCREEN_HEIGHT-300.0f};
	ui3.source={328,126,330,166};
	ui3.lerp=0.05f;
	
	string all_text="qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM.0123456789零件库健康指示能量濒死浮游狂胞";
	int codepointcount;
	int* codepoints=LoadCodepoints(all_text.c_str(),&codepointcount);
	Font font=LoadFontEx("fonts/Madfont.ttf",48,codepoints,codepointcount);
	
	vector<Part> Partlibrary=
	{
		{0,"空",{0,0,32,32},0.0f,0.0f,PartType::none,Organelle::none,0.0f},
		{1,"细胞质块",{192,0,32,32},1.0f,50.0f,PartType::cy_0,Organelle::none,0.1F},
		{2,"鞭毛基底",{224,0,32,32},0.4F,30.0F,PartType::mount,Organelle::none,0.2F},
		{3,"左纤毛",{256,0,32,32},0.2f,20.0f,PartType::cilium_left,Organelle::none,0.3f},
		{4,"右纤毛",{288,0,32,32},0.2f,20.0f,PartType::cilium_right,Organelle::none,0.4f},
		//{5,"细胞核",{32,32,32,32},0.5f,0.0f,PartType::none,Organelle::nucleus,0.5f},
		{5,"线粒体",{64,32,32,32},0.1f,0.0f,PartType::none,Organelle::mitochondrion,0.6f},
		{6,"特化口沟",{96,32,64,32},0.5f,0.0f,PartType::none,Organelle::specialized_oral_groove,0.7f},
	};
	
	bool shouldExit=false;
	while(!WindowShouldClose())
	{
		float deltaT=GetFrameTime();
		switch(interface)//游戏界面
		{
		case GameState::GAMING:
		{
			UpdateGaming(deltaT,interface,gamecamera,assemcamera,mouseworldposition,zooming,Partlibrary,ui1,bgm,ui2,Sound_pool,shoot1,shoot2,hit,broken1,broken2,dong,ui3,di);
			break;
		}
		case GameState::ASSEMBLING:
		{
			UpdateAssembling(deltaT,interface,gamecamera,assemcamera,builder,mouseworldposition,zooming,Partlibrary,ui1,UI1_grid);
			break;
		}
		case GameState::CREATER:
		{
			UpdateCreater(deltaT,ui4);
			break;
		}
		case GameState::MEMU:
		{
			UpdateMenu();
			break;
		}
		default:break;
		}
		BeginDrawing();
		switch(interface)
		{
		case GameState::GAMING:
		{
			DrawGaming(texture_all_parts,texture_ui,background,gamecamera,builder,ui2,font,ui3);
			break;
		}
		case GameState::ASSEMBLING:
		{
			DrawAssembling(texture_all_parts,texture_ui,assemcamera,builder,ui1,UI1_grid,font,Partlibrary);
			break;
		}
		case GameState::CREATER:
		{
			DrawCreater(texture_Dyx,ui4,font);
			break;
		}
		case GameState::MEMU:
		{
			DrawMenu();
			break;
		}
		default:break;
		}
		EndDrawing();
	}
	CloseWindow();
	CloseAudioDevice();
	
	UnloadTexture(background);
	UnloadTexture(texture_all_parts);
	UnloadTexture(texture_ui);
	UnloadTexture(texture_Dyx);
	UnloadSound(shoot1);
	UnloadSound(shoot2);
	UnloadSound(hit);
	UnloadSound(broken1);
	UnloadSound(broken2);
	UnloadSound(dong);
	UnloadSound(di);
	UnloadMusicStream(bgm);
	
	return 0;
}

