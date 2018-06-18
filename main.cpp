#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <cmath>
#include <ctime>
#include <stdio.h>
#include <thread>
#include <cassert>
#include <sstream>

#define SUNVOX_MAIN
#if       _WIN32_WINNT < 0x0500
  #undef  _WIN32_WINNT
  #define _WIN32_WINNT   0x0500
#endif
#include <windows.h>

#include "GL/glew.h"
#include "GL/gl.h"
#include "GL/glut.h"
#include "SDL2/SDL.h"
#include "glm/gtx/quaternion.hpp"

#include "glm/gtx/euler_angles.hpp"
#include <lua.hpp>
#include "sunvox.h"
#include "../include/stb_image.h"
#include "obj_loader.h"
#include "manymouse.h"
#include "AStar.hpp"

#include "fbxscene.h"
#include "loadfbx.h"
#include "fbxmath.h"

#include "MESH.H"
#include "Transform.h"
#include "camera.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "OBJ.h"
#include "DISPLAY.H"

#include "svFunctions.h"

#define WIDTH 1400
#define HEIGHT 800

using namespace std;
template <typename T> string type_name();
using namespace glm;

int sInt(string wat){
    stringstream st;
    st << wat;
    int out;
    st >> out;
    return out;
}

double sDob(string wat){
    stringstream st;
    st << wat;
    double out;
    st >> out;
    return out;
}

string itos(int wat){
    stringstream st;
    st << wat;
    string out;
    st >> out;
    return out;
}

string ftos(double wat){
    stringstream st;
    st << wat;
    string out;
    st >> out;
    return out;
}

vector<string> luaFN;
vector<int(*)(lua_State *L)> luaFF;

struct CamObj{
    CamObj(const glm::vec3& pos, float fov, float aspect, float zNear, float zFar,string na){
        cam.DoCamera(pos,fov,aspect,zNear,zFar);
        name = na;
    }
    CamObj(){}
    Camera cam;
    string name;
};

struct TfObj{
    TfObj(const glm::vec3& pos = glm::vec3(), const glm::vec3& rot = glm::vec3(), const glm::vec3& scale = glm::vec3(1.0f, 1.0f, 1.0f),string na = string("nothing")){
        tf.DoTransform(pos,rot,scale);
        name = na;
    }
    TfObj(string na){
        name = na;
    }
    Transform tf;
    string name;
};

vector<CamObj> cams;
vector<TfObj> tfs;

struct LShader{
    static const unsigned int shNum = 2;
    enum{
        TRANSFORM_U,
        UIPOS_U,
        UISCL_U,
        NUM_UNIFORMS
    };
    GLuint program;
    GLuint shaders[shNum];
    GLuint uniforms[NUM_UNIFORMS];
    string name;
};

struct LMesh{
    enum{
        POSITION_VB,
        TEXCOORD_VB,
        NORMAL_VB,
        INDEX_VB,
        buffNum
    };

    GLuint vertexArrObj;
    GLuint vertexArrBuff[buffNum];
    unsigned int drawCount;
    string name;
};

struct LTexture{
    GLuint texture;
    string name;
};

struct LPath{
    AStar::Generator gen;
    string name;
    int w,h;
};

vector<LPath> paths;

vector<LShader> shaders;
vector<LMesh> models;
vector<LTexture> textures;

struct LEvent {
    LEvent(string a){
        type = a;
    }
    void set(string t = string("tick"), string p1 = string(""), string p2 = string(""), string p3 = string(""), string p4 = string("")){
        type = t;
        param1 = p1;
        param2 = p2;
        param3 = p3;
        param4 = p4;
    }
    string type;
    string param1;
    string param2;
    string param3;
    string param4;
};

vector<LEvent> events;

struct LUniform {
    vector<float> f;
    vector<int> i;
    enum {
        FLOAT_UNIFORM,
        INT_UNIFORM
    };
    int type;
    string name;
};

struct LRenderGroupObject {
    LShader shd;
    LMesh mesh;
    LTexture tex;
    CamObj* cam;
    TfObj tf;

    vector<LUniform> uniforms;

    int x,y,w,h;

    bool hasTexture = false;
    bool hasCamNdTrans = false;
    bool isUI = false;
};

struct LRenderGroup {
    vector<LRenderGroupObject> objs;
    string name;
};

vector<LRenderGroup> rGroups;

void LInitModel(const IndexedModel& model, int index){
    models[index].drawCount = model.indices.size();

    glGenVertexArrays(1,&models[index].vertexArrObj);
    glBindVertexArray(models[index].vertexArrObj);

    glGenBuffers(models[index].buffNum,models[index].vertexArrBuff);
    glBindBuffer(GL_ARRAY_BUFFER,models[index].vertexArrBuff[models[index].POSITION_VB]);
    glBufferData(GL_ARRAY_BUFFER,model.positions.size() * sizeof(model.positions[0]), &model.positions[0], GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, models[index].vertexArrBuff[models[index].TEXCOORD_VB]);
    glBufferData(GL_ARRAY_BUFFER, model.positions.size() * sizeof(model.texCoords[0]), &model.texCoords[0], GL_STATIC_DRAW);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, models[index].vertexArrBuff[models[index].NORMAL_VB]);
    glBufferData(GL_ARRAY_BUFFER, model.normals.size() * sizeof(model.normals[0]), &model.normals[0], GL_STATIC_DRAW);

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, models[index].vertexArrBuff[models[index].INDEX_VB]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, model.indices.size() * sizeof(model.indices[0]), &model.indices[0], GL_STATIC_DRAW);

    glBindVertexArray(0);
}

void LCreateMesh(LLVertex* vertices, unsigned int num, unsigned int* indices, unsigned int numIn, int index){
    IndexedModel model;
    for(unsigned int i=0;i<num;i++){
        model.positions.push_back(vertices[i].pos);
        model.texCoords.push_back(vertices[i].texPos);
        model.normals.push_back(vertices[i].normal);
    }

    for(unsigned int i=0;i<numIn;i++){
        model.indices.push_back(indices[i]);
    }

    LInitModel(model,index);
}

void LCreateMesh(const string& fileName, int index){
    IndexedModel model;
    if(fileName.substr(fileName.length()-4,4)==".fbx"){
        FbxScene scn;
        FbxUtil::Stream fs;
        fs.open(fileName,ios_base::in);
        if(!fs){
            cerr << "Couldn't load FBX: " << fileName << endl;
        }
        LoadFbx(&scn,fs);
        cout << scn.allModels.size()<<endl;
        int counter = 1;
        for (auto& mp :scn.rootModels)
        {
            cout << "MESH" <<  counter << endl;
            counter++;
            FbxScene::Mesh* msh = scn.GetMeshOfModel(mp);
            if(msh){
                for(int i=0;i<msh->Vertices.size();i++){
                    model.positions.push_back(glm::vec3(msh->Vertices[i][0],msh->Vertices[i][1],msh->Vertices[i][2]));
                    //cout << model.positions[i].x <<" "<< model.positions[i].y <<" "<<  model.positions[i].z << endl;
                    model.texCoords.push_back(glm::vec2(msh->renderVertex[i].texcoord[0][0],msh->renderVertex[i].texcoord[0][1]));
                    //cout << model.texCoords[i].x <<" "<< model.texCoords[i].y << endl;
                    model.normals.push_back(glm::vec3(msh->renderVertex[i].normal[0],msh->renderVertex[i].normal[1],msh->renderVertex[i].normal[2]));
                    //cout << model.normals[i].x <<" "<< model.normals[i].y <<" "<<  model.normals[i].z << endl<< endl;
                }
                for(int i=0;i<msh->triangles.size();i++){
                    model.indices.push_back(msh->triangles[i].vertex[0]);
                    model.indices.push_back(msh->triangles[i].vertex[1]);
                    model.indices.push_back(msh->triangles[i].vertex[2]);
                    //cout << msh->triangles[i].vertex[0] << endl;
                }
            }
        }
    } else if(fileName.substr(fileName.length()-4,4)==".obj"){
        IndexedModel del = OBJModel(fileName).ToIndexedModel();
        model = del;
        for(int i = 1;i<del.texCoords.size();i++){
            model.texCoords[i] = vec2(del.texCoords[i].x,del.texCoords[i].y*(-1));
        }
    }

    LInitModel(model,index);
}

void LTextureLoad(const string& fileName,int index){
    int width, height, comp;
    unsigned char* imageData = stbi_load(fileName.c_str(),&width,&height,&comp,4);
    if(imageData == NULL)
        cerr << "Texture failed to load: " << fileName << endl;

    glGenTextures(1, &textures[index].texture);
    glBindTexture(GL_TEXTURE_2D, textures[index].texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);

    stbi_image_free(imageData);
}

Display displ;
bool wind = false;

void print_error(lua_State* L) {
  lua_getfield(L, LUA_GLOBALSINDEX, "debug");
    lua_getfield(L, -1, "traceback");
    lua_pushvalue(L, 1);
    lua_pushinteger(L, 2);
    lua_call(L, 2, 1);
    fprintf(stderr, "%s\n", lua_tostring(L, -1));
}

void addLuaFunc(string name, int(*l)(lua_State *L)){
    luaFN.push_back(name);
    luaFF.push_back(l);
}

time_t lastUpdate;
bool executing = true;
bool toCheck = true;
int timeout = 100;

int setLuaKiller(lua_State *L){
    vector<string> args {"boolean","number"};
    if(checkArgs(L,args)==1){
        timeout = lua_tointeger(L,2);
        toCheck = lua_toboolean(L,1);
    } else {
        lua_pushstring(L,"Wrong args");
        return 1;
    }
}

struct LMotion {
    int x=0,y=0;
    LMotion(int a, int b){
        x = a;
        y = b;
    }
};

vector<LMotion> motions;

void luaKiller(lua_State *L){
    while(executing){
        time_t curr;
        time(&curr);
        if(lastUpdate+timeout<curr&&toCheck){
            cerr << endl << "ERROR! Too long without yielding" << endl;
            luaL_error(L,"Too long without yielding");
        }

        ManyMouseEvent a;
        int x=0,y=0;
        while(ManyMouse_PollEvent(&a)){
            if(a.type == MANYMOUSE_EVENT_RELMOTION){
                if(a.item)
                    y = a.value;
                else
                    x = a.value;
            }
        }
        motions.push_back(LMotion(x,y));
        Sleep(1000/600);
    }
}

string mainLUA = R"(

function print(...)
	local args = { ... }
	for i = 1,#args,1 do
		io.write(tostring(args[i]).."   ")
	end
	io.write("\n")
end



local jgel = {}

local isWindowUp = false

local uiVS = "#version 120 \n attribute vec3 position; attribute vec2 texCoord; attribute vec3 normal; uniform mat4 transform; uniform vec2 uipos; uniform vec2 uiscl;  void main(){ gl_Position = (vec4(position,1.0) + vec4(uipos,1,1)) * vec4(uiscl,1,1); }"
local uiFS = "#version 120 \n uniform vec4 color; void main(){ gl_FragColor = color; }"

local textVS = "#version 120 \n attribute vec3 position; attribute vec2 texCoord; attribute vec3 normal; uniform mat4 transform; void main(){ gl_Position = vec4(position,1.0); }"
local textFS = "#version 120 \n uniform vec4 color; void main(){ gl_FragColor = color; }"

local basicVS = "#version 120 \n attribute vec3 position; attribute vec2 texCoord; attribute vec3 normal; uniform mat4 transform; void main(){ gl_Position = transform * vec4(position,1.0); }"
local basicFS = "#version 120 \n uniform vec4 color; void main(){ gl_FragColor = color; }"

local texVS = "#version 120 \n attribute vec3 position; attribute vec2 texCoord; attribute vec3 normal; uniform mat4 transform; varying vec2 texCoord0; void main(){ gl_Position = transform * vec4(position,1.0); texCoord0 = texCoord; }"
local texFS = "#version 120 \n uniform vec4 color; uniform sampler2D diff; varying vec2 texCoord0; void main(){ gl_FragColor = texture2D(diff,texCoord0) * color; }"

--[[
#define KERNEL_SIZE 9

// Gaussian kernel
// 1 2 1
// 2 4 2
// 1 2 1
float kernel[KERNEL_SIZE];

uniform sampler2D colorMap;
uniform float width;
uniform float height;

float step_w = 1.0/width;
float step_h = 1.0/height;

vec2 offset[KERNEL_SIZE];

void main(void)
{
   int i = 0;
   vec4 sum = vec4(0.0);

   offset[0] = vec2(-step_w, -step_h);
   offset[1] = vec2(0.0, -step_h);
   offset[2] = vec2(step_w, -step_h);

   offset[3] = vec2(-step_w, 0.0);
   offset[4] = vec2(0.0, 0.0);
   offset[5] = vec2(step_w, 0.0);

   offset[6] = vec2(-step_w, step_h);
   offset[7] = vec2(0.0, step_h);
   offset[8] = vec2(step_w, step_h);

   kernel[0] = 1.0/16.0; 	kernel[1] = 2.0/16.0;	kernel[2] = 1.0/16.0;
   kernel[3] = 2.0/16.0;	kernel[4] = 4.0/16.0;	kernel[5] = 2.0/16.0;
   kernel[6] = 1.0/16.0;   	kernel[7] = 2.0/16.0;	kernel[8] = 1.0/16.0;


   if(gl_TexCoord[0].s<0.495)
   {
	   for( i=0; i<KERNEL_SIZE; i++ )
	   {
			vec4 tmp = texture2D(colorMap, gl_TexCoord[0].st + offset[i]);
			sum += tmp * kernel[i];
	   }
   }
   else if( gl_TexCoord[0].s>0.505 )
   {
		sum = texture2D(colorMap, gl_TexCoord[0].xy);
   }
   else
   {
		sum = vec4(1.0, 0.0, 0.0, 1.0);
	}

   gl_FragColor = sum;
}

]]

local function checkWindow()
	if not isWindowUp then
		error("You need to first init the window")
	end
end

function checkArgs(...)
	local args = { ... }
	for i=1,#args,1 do
		if type(args[i]) == "table" then
			local argty = {unpack(args[i])}
			table.remove(argty,1)

			local function concc(ar)
				local str = ""
				for i,v in pairs(ar) do
					if type(i) == "number" then
						str = str .. v
						if i ~= #ar then
							str = str .. " or "
						end
					end
				end
				return str
			end

			local right = false
			for j=1,#argty,1 do
				if type(args[i][1]) == argty[j] then
					right = true
				end
			end
			if not right then error("arg #"..i..": expected "..concc(argty)..", got "..type(args[i][1])) end
		else
			error("Bad arg check, tables needed")
		end
	end
end

function checkTable(tabl,...)
	local args = {...}
	for i=1,#args,1 do
		if type(tabl[i]) ~= tostring(args[i]) then
			error("Wrong table contents, #"..i.." is "..type(tabl[i])..", needed "..tostring(args[i]))
		end
	end
end

local function checkBTable(tabl,...)
	local args = {...}
	for i=1,#args,1 do
		if type(tabl[i]) ~= tostring(args[i]) then
			return false
		end
	end
	return true
end

local function cross(sv,dv)
	checkTable(sv,"number","number","number")
	checkTable(dv,"number","number","number")
	return {sv[2]*dv[3] - sv[3]*dv[2], sv[3]*dv[1] - sv[1]*dv[3],sv[1]*dv[2]-sv[2]*dv[1]}
end

local function dot(sv,dv)
	checkTable(sv,"number","number","number")
	checkTable(dv,"number","number","number")
	return sv[1]*dv[1]+sv[2]*dv[2]+sv[3]*dv[3]
end

local activeCam = {}
local allCameras = {}
local allMObjects = {}
local allTextures = {}
local allText = {}
local allPaths = {}

function jgel.createCamera(Position,FOV,zN,zF)
	checkWindow()
	checkArgs({Position,"table"},{FOV,"number"},{zN,"number"},{zF,"number"})
	checkTable(Position,"number","number","number")

	local cameraID = engine.CameraCreate(Position[1],Position[2],Position[3],FOV,zN,zF,"cam"..#allCameras)
	local cameraName = "cam"..#allCameras
	local Pos,Fov,zNear,zFar,Rot = Position,FOV,zN,zF,{0,0,0}

	local function checkValid()
		if engine.GetName(cameraID,4) ~= cameraName then
			cameraID = engine.GetID(cameraName,4)
		end
	end

	local camera = {Type = "mobj"}

	function camera.setPosition(pos)
		checkArgs({pos,"table"})
		checkTable(pos,"number","number","number")
		checkValid()
		Pos = pos
		engine.CameraSetPosition(pos[1],pos[2],pos[3],cameraID)
	end

	function camera.setRotation(rot)
		checkArgs({rot,"table"})
		checkTable(rot,"number","number","number")
		checkValid()
		Rot = rot
		engine.CameraSetRotation(rot[1],rot[2],rot[3],cameraID)
	end

	function camera.setFOV(fov)
		checkArgs({fov,"number"})
		checkValid()
		Fov = fov
		engine.CameraSetFOV(fov,cameraID)
	end

	function camera.setZRange(zn,zf)
		checkArgs({zN,"number"},{zF,"number"})
		checkValid()
		zNear = zn
		zFar = zf
		engine.CameraSetZRange(zn,zf,cameraID)
	end

	function camera.getPosition()
		checkValid()
		return unpack(Pos)
	end

	function camera.getForward()
		checkValid()
		return engine.CameraGetFwd(cameraID)
	end

	function camera.getLeft()
		checkValid()
		return engine.CameraGetLeft(cameraID)
	end

	function camera.getUp()
		checkValid()
		return engine.CameraGetUp(cameraID)
	end

	function camera.getRotation()
		checkValid()
		return unpack(Rot);
	end

	function camera.getID()
		return cameraID
	end

	function camera.delete()
		checkValid()
		engine.DeleteObject(cameraID,4)
		cameraID = -1
	end

	engine.CameraSetPosition(Position[1],Position[2],Position[3],cameraID)

	if #allCameras == 0 then
		activeCam = camera
	end

	table.insert(allCameras,camera)
	return camera
end

function jgel.createTexture(filename)
	checkWindow()
	checkArgs({filename,"string"})
	local name = "tex"..#allTextures
	local textureID, textureName = -1,name.."tex"
	textureID = engine.TextureCreate(filename,name.."tex")

	local function checkValid()
		if engine.GetName(textureID,3) ~= textureName then
			textureID = engine.GetID(textureName,3)
		end
	end

	local texture = {Type = "texture"}

	function texture.SetTexture(txt)
		checkArgs({txt,"string"})
		checkValid()
		engine.DeleteObject(textureID,3)
		textureID = engine.TextureCreate(txt,name.."tex")
	end

	function texture.getID()
		return textureID
	end

	function texture.delete()
		checkValid()
		engine.DeleteObject(textureID,3)
	end


	table.insert(allTextures,texture)
	return texture
end


function jgel.createMeshObject(Position,Obj,c)
	checkWindow()
	checkArgs({Position,"table"},{Obj,"table"},{Obj,"table"})
	checkTable(Position,"number","number","number")
	checkTable(c,"number","number","number","number")
	if not ( checkBTable(Obj,"table","table") or checkBTable(Obj,"string") ) then
		error("Wrong Object table")
	end

	local ui = false
	local name = "mobj" .. #allMObjects

	local Pos,Rot,Scale,Color = Position,{0,0,0},{1,1,1},c
	local shdType = "basic"
	local meshID,meshName = -1,name.."mesh"

	if Obj[2] then
		meshID = engine.MeshCreate(Obj[1],Obj[2],name.."mesh")
	else
		meshID = engine.MeshCreate(Obj[1],name.."mesh")
	end

	local transformID,transformName = -1,name.."tf"
	transformID = engine.TransformCreate(Position[1],Position[2],Position[3],0,0,0,1,1,1,transformName)

	local textureID = -1

	local cameraID = -1

	if type(activeCam.getID) == "function" then
        cameraID = activeCam.getID()
    end

	local shaderID = engine.ShaderCreate(basicVS,basicFS,name.."shd")
	local shaderName,Type = name.."shd","basic"

	local mo = {Type = "mobj", uniforms = {}}

	local function checkValid()
		if engine.GetName(meshID,2) ~= meshName then
			meshID = engine.GetID(meshName,2)
		end
		if engine.GetName(transformID,5) ~= transformName then
			transformID = engine.GetID(transformName,5)
		end
		if engine.GetName(shaderID,1) ~= shaderName then
			shaderID = engine.GetID(shaderName,1)
		end
	end

	function mo.setCamera(camm)
        if type(camm.getID) == "function" then
            cameraID = camm.getID()
        end
    end

	function mo.draw()
		if type(activeCam.getID) == "function" and cameraID == -1 then
			engine.ShaderBind(shaderID)
			if textureID ~= -1 then
                engine.TextureBind(textureID,0)
            end
			engine.UniformF(Color[1],Color[2],Color[3],Color[4],"color")

			for i,v in pairs(mo.uniforms) do
                if v.type == "int" then
                    if v.count == 1 then
                        engine.UniformI(v[1],i)
                    elseif v.count == 2 then
                        engine.UniformI(v[1],v[2],i)
                    elseif v.count == 3 then
                        engine.UniformI(v[1],v[2],v[3],i)
                    elseif v.count == 4 then
                        engine.UniformI(v[1],v[2],v[3],v[4],i)
                    end
                else
                    if v.count == 1 then
                        engine.UniformF(v[1],i)
                    elseif v.count == 2 then
                        engine.UniformF(v[1],v[2],i)
                    elseif v.count == 3 then
                        engine.UniformF(v[1],v[2],v[3],i)
                    elseif v.count == 4 then
                        engine.UniformF(v[1],v[2],v[3],v[4],i)
                    end
                end
            end

			engine.ShaderUpdate(shaderID,activeCam.getID(),transformID)
			if ui then
				engine.ShaderUpdateUI(shaderID,Pos[1],Pos[2],Scale[1],Scale[2])
			end
			engine.MeshDraw(meshID)
		elseif cameraID > -1 then
            engine.ShaderBind(shaderID)
			if textureID ~= -1 then
                engine.TextureBind(textureID,0)
            end
			engine.UniformF(Color[1],Color[2],Color[3],Color[4],"color")

			for i,v in pairs(mo.uniforms) do
                if v.type == "int" then
                    if v.count == 1 then
                        engine.UniformI(v[1],i)
                    elseif v.count == 2 then
                        engine.UniformI(v[1],v[2],i)
                    elseif v.count == 3 then
                        engine.UniformI(v[1],v[2],v[3],i)
                    elseif v.count == 4 then
                        engine.UniformI(v[1],v[2],v[3],v[4],i)
                    end
                else
                    if v.count == 1 then
                        engine.UniformF(v[1],i)
                    elseif v.count == 2 then
                        engine.UniformF(v[1],v[2],i)
                    elseif v.count == 3 then
                        engine.UniformF(v[1],v[2],v[3],i)
                    elseif v.count == 4 then
                        engine.UniformF(v[1],v[2],v[3],v[4],i)
                    end
                end
            end

			engine.ShaderUpdate(shaderID,cameraID,transformID)
			if ui then
				engine.ShaderUpdateUI(shaderID,Pos[1],Pos[2],Scale[1],Scale[2])
			end
			engine.MeshDraw(meshID)
        end
	end

	function mo.setPosition(pos)
		checkArgs({pos,"table"})
		checkTable(pos,"number","number","number")
		checkValid()
		Pos = pos
		engine.TransformSetPos(pos[1],pos[2],pos[3],transformID)
	end

	function mo.setRotation(pos)
		checkArgs({pos,"table"})
		checkTable(pos,"number","number","number")
		checkValid()
		Rot = pos
		engine.TransformSetRot(pos[1],pos[2],pos[3],transformID)
	end

	function mo.setScale(pos)
		checkArgs({pos,"table"})
		checkTable(pos,"number","number","number")
		checkValid()
		Scale = pos
		engine.TransformSetScale(pos[1],pos[2],pos[3],transformID)
	end

	function mo.setColor(color)
		checkArgs({color,"table"})
		checkTable(color,"number","number","number","number")
		Color = color
	end

	function mo.setTexture(txty)
		if txty.getID then
			textureID = txty.getID()
		else
			print("Wrong texture object")
		end
	end

	function mo.createBasicShader()
		checkValid()
		shdType = "basic"
		engine.DeleteObject(shaderName,1)
		shaderID = engine.ShaderCreate(basicVS,basicFS,name.."shd")
		ui = false
	end

	function mo.createTexturedShader()
		checkValid()
		shdType = "texture"
		engine.DeleteObject(shaderName,1)
		shaderID = engine.ShaderCreate(texVS,texFS,name.."shd")
		ui = false
	end

	function mo.createUIShader()
		checkValid()
		shdType = "ui"
		engine.DeleteObject(shaderName,1)
		shaderID = engine.ShaderCreate(uiVS,uiFS,name.."shd")
		ui = true
	end



	local xpos,xrot,xscl,xc,xmi,xmn,xtype,xtex = Pos,Rot,Scale,Color,meshID,meshName,shdType,textureID
	function mo.copy()
		local ui = false
		local name = "mobj" .. #allMObjects

		local shdType = xtype
		local Pos,Rot,Scale,Color = xpos,xrot,xscl,xc
		local meshID,meshName = xmi,xmn

		local transformID,transformName = -1,name.."tf"
		transformID = engine.TransformCreate(Position[1],Position[2],Position[3],0,0,0,1,1,1,transformName)

		local textureID = xtex

		local shaderID = -1
		local shaderName = name.."shd"
		if shdType == "basic" then
			shaderID = engine.ShaderCreate(basicVS,basicFS,name.."shd")
		elseif shdType == "texture" then
			shaderID = engine.ShaderCreate(texVS,texFS,name.."shd")
			print("textured")
		elseif shdType == "ui" then
			shaderID = engine.ShaderCreate(uiVS,uiFS,name.."shd")
		end

		local mo = {Type = "mobj"}

		local function checkValid()
			if engine.GetName(meshID,2) ~= meshName then
				meshID = engine.GetID(meshName,2)
			end
			if engine.GetName(transformID,5) ~= transformName then
				transformID = engine.GetID(transformName,5)
			end
			if engine.GetName(shaderID,1) ~= shaderName then
				shaderID = engine.GetID(shaderName,1)
			end
		end

		function mo.draw()
			if type(activeCam.getID) == "function" and cameraID == -1 then
                engine.ShaderBind(shaderID)
                if textureID ~= -1 then
                    engine.TextureBind(textureID,0)
                end
                engine.UniformF(Color[1],Color[2],Color[3],Color[4],"color")

                for i,v in pairs(mo.uniforms) do
                    if v.type == "int" then
                        if v.count == 1 then
                            engine.UniformI(v[1],i)
                        elseif v.count == 2 then
                            engine.UniformI(v[1],v[2],i)
                        elseif v.count == 3 then
                            engine.UniformI(v[1],v[2],v[3],i)
                        elseif v.count == 4 then
                            engine.UniformI(v[1],v[2],v[3],v[4],i)
                        end
                    else
                        if v.count == 1 then
                            engine.UniformF(v[1],i)
                        elseif v.count == 2 then
                            engine.UniformF(v[1],v[2],i)
                        elseif v.count == 3 then
                            engine.UniformF(v[1],v[2],v[3],i)
                        elseif v.count == 4 then
                            engine.UniformF(v[1],v[2],v[3],v[4],i)
                        end
                    end
                end

                engine.ShaderUpdate(shaderID,activeCam.getID(),transformID)
                if ui then
                    engine.ShaderUpdateUI(shaderID,Pos[1],Pos[2],Scale[1],Scale[2])
                end
                engine.MeshDraw(meshID)
            elseif cameraID > -1 then
                engine.ShaderBind(shaderID)
                if textureID ~= -1 then
                    engine.TextureBind(textureID,0)
                end
                engine.UniformF(Color[1],Color[2],Color[3],Color[4],"color")

                for i,v in pairs(mo.uniforms) do
                    if v.type == "int" then
                        if v.count == 1 then
                            engine.UniformI(v[1],i)
                        elseif v.count == 2 then
                            engine.UniformI(v[1],v[2],i)
                        elseif v.count == 3 then
                            engine.UniformI(v[1],v[2],v[3],i)
                        elseif v.count == 4 then
                            engine.UniformI(v[1],v[2],v[3],v[4],i)
                        end
                    else
                        if v.count == 1 then
                            engine.UniformF(v[1],i)
                        elseif v.count == 2 then
                            engine.UniformF(v[1],v[2],i)
                        elseif v.count == 3 then
                            engine.UniformF(v[1],v[2],v[3],i)
                        elseif v.count == 4 then
                            engine.UniformF(v[1],v[2],v[3],v[4],i)
                        end
                    end
                end

                engine.ShaderUpdate(shaderID,cameraID,transformID)
                if ui then
                    engine.ShaderUpdateUI(shaderID,Pos[1],Pos[2],Scale[1],Scale[2])
                end
                engine.MeshDraw(meshID)
            end
		end

		function mo.setPosition(pos)
			checkArgs({pos,"table"})
			checkTable(pos,"number","number","number")
			checkValid()
			Pos = pos
			engine.TransformSetPos(pos[1],pos[2],pos[3],transformID)
		end

		function mo.setRotation(pos)
			checkArgs({pos,"table"})
			checkTable(pos,"number","number","number")
			checkValid()
			Rot = pos
			engine.TransformSetRot(pos[1],pos[2],pos[3],transformID)
		end

		function mo.setScale(pos)
			checkArgs({pos,"table"})
			checkTable(pos,"number","number","number")
			checkValid()
			Scale = pos
			engine.TransformSetScale(pos[1],pos[2],pos[3],transformID)
		end

		function mo.setColor(color)
			checkArgs({color,"table"})
			checkTable(color,"number","number","number","number")
			Color = color
		end

		function mo.setTexture(txty)
			if txty.getID then
				textureID = txty.getID()
			else
				print("Wrong texture object")
			end
		end

		function mo.setCamera(camm)
            if type(camm.getID) == "function" then
                cameraID = camm.getID()
            end
        end

		function mo.createBasicShader()
			checkValid()
			shdType = "basic"
			engine.DeleteObject(shaderName,1)
			shaderID = engine.ShaderCreate(basicVS,basicFS,name.."shd")
			ui = false
		end

		function mo.createTexturedShader()
			checkValid()
			shdType = "texture"
			engine.DeleteObject(shaderName,1)
			shaderID = engine.ShaderCreate(texVS,texFS,name.."shd")
			ui = false
		end

		function mo.createUIShader()
			checkValid()
			shdType = "ui"
			engine.DeleteObject(shaderName,1)
			shaderID = engine.ShaderCreate(uiVS,uiFS,name.."shd")
			ui = true
		end

		function mo.createCustomShader(vs,fs)
			checkValid()
			shdType = "custom"
			engine.DeleteObject(shaderName,1)
			shaderID = engine.ShaderCreate(vs,fs,name.."shd")
			ui = false
		end

		function mo.getShaderType()
            return shdType
        end

		function mo.delete()
			checkValid()
			engine.DeleteObject(shaderID,1)
			engine.DeleteObject(meshID,2)
			engine.DeleteObject(transformID,5)
		end

		table.insert(allMObjects,mo)
		return mo
	end

function mo.createCustomShader(vs,fs)
			checkValid()
			shdType = "custom"
			engine.DeleteObject(shaderName,1)
			shaderID = engine.ShaderCreate(vs,fs,name.."shd")
			ui = false
		end





	function mo.delete()
		checkValid()
		engine.DeleteObject(shaderID,1)
		engine.DeleteObject(meshID,2)
		engine.DeleteObject(transformID,5)
	end

	table.insert(allMObjects,mo)
	return mo
end

function jgel.createText(text,x,y,c)
	checkWindow()
	checkArgs({text,"string"},{x,"number"},{y,"number"},{c,"table"})
	checkTable(c,"number","number","number","number")
	local name = "text" .. #allText
	local Type = "text"
	local shaderID,shaderName = -1,""
	local texty = text
	local Position = {x,y}
	local color = c
	shaderID = engine.ShaderCreate(textVS,textFS,name.."shd")
	shaderName = name.."shd"

	local function checkValid()
		if engine.GetName(shaderID,1) ~= shaderName then
			shaderID = engine.GetID(shaderName,1)
		end
	end

	local text = {}

	function text.draw()
		checkValid()
		engine.ShaderBind(shaderID)
		engine.UniformF(color[1],color[2],color[3],color[4],"color")
		engine.TextDraw(texty,Position[1],Position[2])
	end

	function text.setText(te)
		checkArgs({te,"string"})
		texty = te
	end

	function text.setPosition(x,y)
		checkArgs({x,"number"},{y,"number"})
		Position = {x,y}
	end

	function text.delete()
		checkValid()
		engine.DeleteObject(shaderID,1)
	end

	table.insert(allText,text)
	return text
end

function jgel.createThread(code)
    local id = -1

    local _,err = loadstring(code)
    if err then
        print(err)
        return nil
    end

    id = engine.runThread(code)

    local thread = {}

    function thread.send(...)
        engine.send(id,unpack({...}))
    end

    return thread
end

local toCall = {}

jgel.subToEvents = function(func) if type(func) == "function" then table.insert(toCall,func) else error("expected function, got "..type(func)) end end

jgel.pullEvent = function() local a = {engine.pullEvent()} for i,v in pairs(toCall) do v(unpack(a)) end return unpack(a) end
jgel.displayClear = function(a,b,c,d) checkArgs({a,"number"},{b,"number"},{c,"number"},{d,"number"}) engine.DisplayClear(a,b,c,d) end
jgel.isDone = engine.isDone
jgel.initWindow = function(a,b,c,d) checkArgs({a,"number"},{b,"number"},{c,"string"},{d,"boolean"}) engine.initWindow(a,b,c,d) isWindowUp = true end

jgel.mouse = {}
jgel.mouse.getMousePos = engine.getMousePos
jgel.mouse.setMousePos = function(x,y) checkArgs({x,"number"},{y,"number"}) engine.setMousePos(x,y) end
jgel.mouse.getMouseInput = engine.getMouseVel

jgel.keyboard = {}
jgel.keyboard.isKeyDown = function(k) checkArgs({k,"string"}) return engine.isKeyDown(k) end
jgel.keyboard.getKeyEvent = engine.getKeyEvent

jgel.sleep = function(t) checkArgs({t,"number"}) engine.sleep(t) end
jgel.setLuaKiller = function(b,n) checkArgs({b,"boolean"},{n,"number"}) engine.setLuaKiller(b,n) end
jgel.FPSLimit = function(g) checkArgs({g,"number"}) jgel.sleep(1000/g) end
jgel.sunvox = {}

for i,v in pairs(engine) do
	if(i:sub(1,2) == "sv") then
		jgel.sunvox[i:sub(3)] = v
	end
end

jgel.math = {}

local quat = {
				__mul = function (t1,t2)
						checkArgs({t1,"table"})
						checkTable(t1,"number","number","number","number")


						checkArgs({t2,"table"})
						checkTable(t2,"number","number","number","number")

						r = {}
						for i = 1,4,1 do
							r[i] = t1[i] * t2[i]
						end
						return r
					end,
				__add = function (t1,t2)
						checkArgs({t1,"table"})
						checkTable(t1,"number","number","number","number")


						checkArgs({t2,"table"})
						checkTable(t2,"number","number","number","number")

						r = {}
						for i = 1,4,1 do
							r[i] = t1[i] + t2[i]
						end
						return r
					end
			}

function jgel.math.rotToQuat(v)
	checkArgs({v,"table"})
	checkTable(v,"number","number","number")
	local c1,c2,c3,s1,s2,s3 = math.cos(v[1]/2),math.cos(v[2]/2),math.cos(v[3]/2),math.sin(v[1]/2),math.sin(v[2]/2),math.sin(v[3]/2)
	return setmetatable({
		c1*c2*c3 - s1*s2*s3,
		s1*s2*c3 + c1*c2*s3,
		s1*c2*c3 + c1*s2*s3,
		c1*s2*c3 - s1*c2*s3
	},quat)
end



local mat = {
				__mul = function (t1,t2)
						checkArgs({t1,"table"})
						checkTable(t1,"table","table","table","table")
						checkTable(t1[1],"number","number","number","number")
						checkTable(t1[2],"number","number","number","number")
						checkTable(t1[3],"number","number","number","number")
						checkTable(t1[4],"number","number","number","number")

						checkArgs({t2,"table"})
						checkTable(t2,"table","table","table","table")
						checkTable(t2[1],"number","number","number","number")
						checkTable(t2[2],"number","number","number","number")
						checkTable(t2[3],"number","number","number","number")
						checkTable(t2[4],"number","number","number","number")

						r = {{},{},{},{}}
						for i = 1,4,1 do
							for j = 1,4,1 do
								r[i][j] = t1[i][j] * t2[i][j]
							end
						end
						return r
					end,
				__add = function (t1,t2)
						checkArgs({t1,"table"})
						checkTable(t1,"table","table","table","table")
						checkTable(t1[1],"number","number","number","number")
						checkTable(t1[2],"number","number","number","number")
						checkTable(t1[3],"number","number","number","number")
						checkTable(t1[4],"number","number","number","number")

						checkArgs({t2,"table"})
						checkTable(t2,"table","table","table","table")
						checkTable(t2[1],"number","number","number","number")
						checkTable(t2[2],"number","number","number","number")
						checkTable(t2[3],"number","number","number","number")
						checkTable(t2[4],"number","number","number","number")

						r = {{},{},{},{}}
						for i = 1,4,1 do
							for j = 1,4,1 do
								r[i][j] = t1[i][j] + t2[i][j]
							end
						end
						return r
					end
			}

function jgel.math.quatToRot(v)
	checkArgs({v,"table"})
	checkTable(v,"number","number","number","number")
	local w,x,y,z = v[1],v[2],v[3],v[4]
	return {
		math.atan2( 2*y*w-2*x*z, 1 - 2*y*y - 2*z*z),
		math.asin( 2*x*y + 2*z*w ),
		math.atan2( 2*x*w-2*y*z , 1-2*x*x - 2*z*z)
	}
end

function jgel.math.quatToMat(q)
	checkArgs({q,"table"})
	checkTable(q,"number","number","number","number")

	local w,x,y,z = q[1],q[2],q[3],q[4]
	return setmetatable({
			{1-2*y*y - 2*z*z, 2*x*y - 2*z*w, 2*x*z+2*y*w,0},
			{2*x*y+2*z*w, 1-2*x*x - 2*z*z, 2*y*z - 2*x*w,0},
			{2*x*z-2*y*w, 2*y*z+2*x*w, 1-2*x*x-2*y*y,0},
			{0,0,0,1}
		},mat)
end

function jgel.math.matToQuat(q)
	checkArgs({q,"table"})
	checkTable(q,"table","table","table","table")
	checkTable(q[1],"number","number","number","number")
	checkTable(q[2],"number","number","number","number")
	checkTable(q[3],"number","number","number","number")
	checkTable(q[4],"number","number","number","number")
	local w = math.sqrt(1+q[1][1]+q[2][2]+q[3][3])/2
	return setmetatable({
			-w,
			-(q[3][2] - q[2][3]) / (4*w),
			-(q[1][3] - q[3][1]) / (4*w),
			-(q[2][1] - q[1][2]) / (4*w)
		},quat)
end

function jgel.math.matInverse(m)
	checkArgs({m,"table"})
	checkTable(m,"table","table","table","table")
	checkTable(m[1],"number","number","number","number")
	checkTable(m[2],"number","number","number","number")
	checkTable(m[3],"number","number","number","number")
	checkTable(m[4],"number","number","number","number")

	local r = {{},{},{},{}}
	for i=1,4,1 do
		for j=1,4,1 do
			r[i][j] = m[i][j] * -1
		end
	end
	return setmetatable(r,mat)
end

function jgel.math.quatInverse(m)
	checkArgs({m,"table"})
	checkTable(m,"number","number","number","number")


	local r = {}
	for i=1,4,1 do
		r[i] = m[i] * -1
	end
	return setmetatable(r,quat)
end

_G.jgel = jgel

--/8kaghsf47582thwaroig35u12905uy83thn0139thyb/

local g,lol = loadfile("game.lua")
if type(g) ~= "function" then
	error(lol)
end
local trac = ""
local ok, err = xpcall(g,function(err) print(err) print(debug.traceback()) end)
print(ok,err)

for i,v in pairs(allCameras) do
    v.delete()
end

for i,v in pairs(allMObjects) do
    v.delete()
end

for i,v in pairs(allTextures) do
    v.delete()
end

for i,v in pairs(allText) do
    v.delete()
end

-- YOUR CODE DOWN HERE, EVERYTHING ABOVE IS THE API
)";

void execute(const char* filename)
{
    lua_State *state = luaL_newstate();
    lua_newtable(state);
    for(int i=0;i<luaFN.size();i++){
        lua_pushcfunction(state,luaFF[i]);
        lua_setfield(state,-2,luaFN[i].c_str());
    }
    lua_setglobal(state,"engine");
    luaL_openlibs(state);
    luaL_loadstring(state, filename);
    cout << lua_tostring(state,-1) << endl;
    time(&lastUpdate);
    thread aksu(luaKiller,state);
    aksu.detach();
    int result = lua_pcall(state, 0, LUA_MULTRET, 0);
    if ( result != 0 ) {
        print_error(state);
        return;
    }
}

int threadCount=0;

struct LThread {
    lua_State *parent;
    lua_State *me;
    string name;
};

vector<LThread> threads;

int sendBack(lua_State *L){
    vector<string> args {"number"};
    if(checkArgsUnDef(L,args)==1){
        int index = lua_tonumber(L,1);
        lua_State *parent = threads[index].parent;
        lua_getglobal(parent,"receive");
        int lena = lua_gettop(L)-1;
        if(lua_isfunction(parent,-1)){
            lua_xmove(L,parent,lena);
            lua_call(parent,lena,0);
        }
    } else {
        lua_pushstring(L,"Wrong args");
        return 1;
    }
}


int send(lua_State *L){
    vector<string> args {"number"};
    if(checkArgsUnDef(L,args)==1){
        int index = lua_tonumber(L,1);

        if(index<0&&index>threads.size()-1){
            lua_pushstring(L,"Out of bounds");
            return 1;
        }

        fprintf(stderr,"\n");
        lua_State *parent = threads[index].me;
        lua_getglobal(parent,"receive");
        int lena = lua_gettop(L)-1;
        if(lua_isfunction(parent,-1)){
            lua_xmove(L,parent,lena);
            lua_call(parent,lena,0);
        }
        lua_pop(L,1);
    } else {
        lua_pushstring(L,"Wrong args");
        return 1;
    }
}

void runLThreat(const char* lol, lua_State *L, string name){
    lua_State *state = luaL_newstate();
    lua_newtable(state);
    for(int i=0;i<luaFN.size();i++){
        lua_pushcfunction(state,luaFF[i]);
        lua_setfield(state,-2,luaFN[i].c_str());
    }
    lua_setglobal(state,"engine");
    luaL_openlibs(state);
    fprintf(stderr, "%s\n", lol);
    string code = "function sendBack(...) _sb(";
    code += itos(threads.size());
    code += ",unpack({...})) end \n ";
    code += lol;

    string API = mainLUA.substr(0,mainLUA.find("--/8kaghsf47582thwaroig35u12905uy83thn0139thyb/")-1);
    luaL_loadstring(state,API.c_str());
    int result = lua_pcall(state,0,LUA_MULTRET,0);
    if ( result != 0 ) {
        print_error(state);
        return;
    }

    luaL_loadstring(state,code.c_str());
    fprintf(stderr, "%s\n", lua_tostring(state, -1));

    lua_register(state,"_sb",sendBack);

    LThread a;
    a.parent = L;
    a.me = state;
    a.name = name;
    threadCount++;
    threads.push_back(a);

    result = lua_pcall(state,0,LUA_MULTRET,0);
    if ( result != 0 ) {
        print_error(state);
        return;
    }
}

int runThreat(lua_State *L){
    vector<string> args {"string"};
    if(checkArgs(L,args)==1){
        lua_pushinteger(L,threads.size());
        thread neww(runLThreat,lua_tostring(L,1),L,string("thread")+itos(threadCount));
        neww.detach();
        Sleep(500);
        return 1;
    } else {
        lua_pushstring(L,"Wrong args");
        return 1;
    }
}



vector<string> keyEvents;

int isKeyDown(lua_State *L){
    vector<string> args {"string"};
    if(checkArgs(L,args)==1){
        lua_pushboolean(L,SDL_GetKeyboardState(NULL)[SDL_GetScancodeFromName(lua_tostring(L,1))] == 1);
        return 1;
    } else {
        lua_pushstring(L,"Wrong args");
        return 1;
    }
}

int getKeyEvent(lua_State *L){
    if(keyEvents.size()>0){
        lua_pushstring(L,keyEvents[0].c_str());
        keyEvents.erase(keyEvents.begin());
        return 1;
    }
}


int getMousePos(lua_State *L){
    vector<string> args {};
    if(checkArgs(L,args)==1){
        int x,y;
        SDL_GetMouseState(&x,&y);
        lua_pushinteger(L,x);
        lua_pushinteger(L,y);
        return 2;
    } else {
        lua_pushstring(L,"Wrong args");
        return 1;
    }
}



int getMouseVel(lua_State *L){
    vector<string> args {};
    if(checkArgs(L,args)==1){
        int x=0,y=0;
        for(int i=0;i<motions.size();i++){
            x += motions[i].x;
            y += motions[i].y;
        }
        motions.clear();
        lua_pushinteger(L,x);
        lua_pushinteger(L,y);
        return 2;
    } else {
        lua_pushstring(L,"Wrong args");
        return 1;
    }
}

int showMouse(lua_State *L){
    vector<string> args {"boolean"};
    if(checkArgs(L,args)==1){
        SDL_ShowCursor((int)lua_toboolean(L,1));
        return 0;
    } else {
        lua_pushstring(L,"Wrong args");
        return 1;
    }
}

int setMousePos(lua_State *L){
    vector<string> args {"number","number"};
    if(checkArgs(L,args)==1){
        if(displ.focused)
            SDL_WarpMouseInWindow(displ.wind,lua_tointeger(L,1),lua_tointeger(L,2));
        return 0;
    } else {
        lua_pushstring(L,"Wrong args");
        return 1;
    }
}

string oldKey = "";

int sleep(lua_State *L){
    vector<string> args {"number"};
    if(checkArgs(L,args)==1){
        int timee = lua_tointeger(L,1);
        lastUpdate += timee;
        Sleep(timee);

        return 0;
    } else {
        lua_pushstring(L,"Wrong args");
        return 1;
    }
}

int pullEvent(lua_State *L){
    time_t curr;
    time(&curr);
    lastUpdate = curr;
    if(wind){
        displ.Update();


        SDL_Event e;
        LEvent newEvent("tick");
        string last;
        while(SDL_PollEvent(&e)){
            if(e.type == SDL_QUIT){
                newEvent.set("quit");
                events.push_back(newEvent);
                displ.setDone(true);
                break;
            } else if(e.type == SDL_WINDOWEVENT){
                if(e.window.event == SDL_WINDOWEVENT_FOCUS_GAINED){
                    displ.focused = true;
                    cout << "focused!\n";
                } else if(e.window.event == SDL_WINDOWEVENT_FOCUS_LOST){
                    displ.focused = false;
                }
            } else if(e.type == SDL_KEYDOWN){
                if(string(SDL_GetScancodeName(e.key.keysym.scancode)) != last){
                    last = string(SDL_GetScancodeName(e.key.keysym.scancode));
                    keyEvents.push_back( string(SDL_GetScancodeName(e.key.keysym.scancode)) + ".down");
                }
            } else if(e.type == SDL_KEYUP){
                keyEvents.push_back( string(SDL_GetScancodeName(e.key.keysym.scancode)) + ".up");
                last = "";
            } else if(e.type == SDL_MOUSEBUTTONDOWN){
                newEvent.set("mousedown",itos(e.button.x),itos(e.button.y),itos(e.button.button));
                events.push_back(newEvent);
            } else if(e.type == SDL_MOUSEBUTTONUP){
                newEvent.set("mouseup",itos(e.button.x),itos(e.button.y),itos(e.button.button));
                events.push_back(newEvent);
            }


             /* else if(e.type == SDL_MOUSEMOTION){
                newEvent.set("mouse_move",itos(e.motion.x),itos(e.motion.y),itos(e.motion.xrel),itos(e.motion.yrel));
                events.push_back(newEvent);
                break;
            }*/

        }

        LEvent viewEv("nothing");

        if(events.size()){
            viewEv.type = events[0].type;
            viewEv.param1 = events[0].param1;
            viewEv.param2 = events[0].param2;
            viewEv.param3 = events[0].param3;
            viewEv.param4 = events[0].param4;
            events.erase(events.begin());
        }

        lua_pushstring(L,viewEv.type.c_str());
        lua_pushstring(L,viewEv.param1.c_str());
        lua_pushstring(L,viewEv.param2.c_str());
        lua_pushstring(L,viewEv.param3.c_str());
        lua_pushstring(L,viewEv.param4.c_str());
        return 5;
    } else
        return 0;
}

int isDone(lua_State *L){
    lua_pushboolean(L,displ.isDone());
    return 1;
}

int initWindow(lua_State *L){
    if(!wind){
        vector<string> types {"number","number","string","boolean"};
        if(checkArgs(L,types)==1){
            displ.Init(lua_tointeger(L,1),lua_tointeger(L,2),lua_tostring(L,3),lua_toboolean(L,4));
            wind = true;
            lua_pushinteger(L,1);
            return 1;
        } else {
            lua_pushinteger(L,0);
            lua_pushstring(L,"Wrong args");
            return 2;
        }
    } else {
        lua_pushinteger(L,0);
        lua_pushstring(L,"Window is already initialized");
        return 2;
    }
}

int MeshCreate(lua_State *L){
    vector<string> args {"table","table","string"};
    if(checkArgs(L,args)==1){
        unsigned int aLen = (unsigned int)lua_objlen(L,1);
        unsigned int bLen = (unsigned int)lua_objlen(L,2);
        LLVertex a[aLen];
        unsigned int b[bLen];
        lua_pushnil(L);
        int indexx = 0;
        while (lua_next(L, 1) != 0) {
            if(lua_istable(L,-1)){
                float px = 0,py = 0,pz = 0,tx = 0,ty = 0,nx = 0,ny = 0,nz = 0;
                if(lua_objlen(L,-1)!=8){
                    lua_pushinteger(L,0);
                    lua_pushstring(L,"Wrong table");
                    return 2;
                }
                lua_pushinteger(L,1);
                lua_gettable(L,-2);
                if(lua_isnumber(L,-1)){
                    px = (float)lua_tonumber(L,-1);
                } else {
                    lua_pushinteger(L,0);
                    lua_pushstring(L,"Wrong table");
                    return 2;
                }
                lua_pop(L,1);

                lua_pushinteger(L,2);
                lua_gettable(L,-2);
                if(lua_isnumber(L,-1)){
                    py = (float)lua_tonumber(L,-1);
                } else {
                    lua_pushinteger(L,0);
                    lua_pushstring(L,"Wrong table");
                    return 2;
                }
                lua_pop(L,1);

                lua_pushinteger(L,3);
                lua_gettable(L,-2);
                if(lua_isnumber(L,-1)){
                    pz = (float)lua_tonumber(L,-1);
                } else {
                    lua_pushinteger(L,0);
                    lua_pushstring(L,"Wrong table");
                    return 2;
                }
                lua_pop(L,1);

                lua_pushinteger(L,4);
                lua_gettable(L,-2);
                if(lua_isnumber(L,-1)){
                    tx = (float)lua_tonumber(L,-1);
                } else {
                    lua_pushinteger(L,0);
                    lua_pushstring(L,"Wrong table");
                    return 2;
                }
                lua_pop(L,1);

                lua_pushinteger(L,5);
                lua_gettable(L,-2);
                if(lua_isnumber(L,-1)){
                    ty = (float)lua_tonumber(L,-1);
                } else {
                    lua_pushinteger(L,0);
                    lua_pushstring(L,"Wrong table");
                    return 2;
                }
                lua_pop(L,1);

                lua_pushinteger(L,6);
                lua_gettable(L,-2);
                if(lua_isnumber(L,-1)){
                    nx = (float)lua_tonumber(L,-1);
                } else {
                    lua_pushinteger(L,0);
                    lua_pushstring(L,"Wrong table");
                    return 2;
                }
                lua_pop(L,1);

                lua_pushinteger(L,7);
                lua_gettable(L,-2);
                if(lua_isnumber(L,-1)){
                    ny = (float)lua_tonumber(L,-1);
                } else {
                    lua_pushinteger(L,0);
                    lua_pushstring(L,"Wrong table");
                    return 2;
                }
                lua_pop(L,1);

                lua_pushinteger(L,8);
                lua_gettable(L,-2);
                if(lua_isnumber(L,-1)){
                    nz = (float)lua_tonumber(L,-1);
                } else {
                    lua_pushinteger(L,0);
                    lua_pushstring(L,"Wrong table");
                    return 2;
                }
                lua_pop(L,1);

                a[indexx].pos.x = px;
                a[indexx].pos.y = py;
                a[indexx].pos.z = pz;
                a[indexx].texPos.x = tx;
                a[indexx].texPos.y = ty;
                a[indexx].normal.x = nx;
                a[indexx].normal.y = ny;
                a[indexx].normal.z = nz;
            } else {
                lua_pushinteger(L,0);
                lua_pushstring(L,"Wrong table");
                return 2;
            }
            lua_pop(L, 1);
            indexx++;
         }
        lua_pushnil(L);
        indexx = 0;
        while (lua_next(L, 2) != 0) {
            if(lua_isnumber(L,-1)){
                b[indexx] = lua_tointeger(L,-1)-1;
            } else {
                lua_pushinteger(L,0);
                lua_pushstring(L,"Wrong table");
                return 2;
            }
            lua_pop(L, 1);
            indexx++;
         }
         if(aLen>0&&bLen>0){
             if(wind){
                LMesh neww;
                neww.name = lua_tostring(L,3);
                int index = models.size();
                models.push_back(neww);
                LCreateMesh(a,aLen,b,bLen,index);
                lua_pushinteger(L,index);
                return 1;
             } else {
                 lua_pushinteger(L,0);
                 lua_pushstring(L,"Init damn window, will you?");
                 return 2;
             }
             return 1;
         } else {
            lua_pushinteger(L,0);
            lua_pushstring(L,"Empty tables");
            return 2;
         }
    } else {
        vector<string> args2 {"string","string"};
        if(checkArgs(L,args2)==1){
            if(wind){
                LMesh neww;
                neww.name = lua_tostring(L,2);
                int index = models.size();
                models.push_back(neww);
                LCreateMesh(lua_tostring(L,1),index);
                lua_pushinteger(L,index);
                return 1;
            } else {
                 lua_pushinteger(L,0);
                 lua_pushstring(L,"Init damn window, will you?");
                 return 2;
             }
        } else {
            lua_pushinteger(L,0);
            lua_pushstring(L,"Wrong args");
            return 2;
        }
    }
}

int MeshDraw(lua_State *L){
    vector<string> args {"number"};
    if(checkArgs(L,args)==1){
        int i = lua_tointeger(L,1);

        if(i<0&&i>models.size()-1){
            lua_pushstring(L,"Out of bounds");
            return 1;
        }

        glBindVertexArray(models[i].vertexArrObj);
        glDrawElements(GL_TRIANGLES, models[i].drawCount, GL_UNSIGNED_INT, 0);
        //glDrawArrays(GL_TRIANGLES, 0, drawCount);
        glBindVertexArray(0);
        return 0;
    } else {
        lua_pushstring(L,"Wrong args");
        return 1;
    }
}

int MeshCopy(lua_State *L){
    vector<string> args {"number","string"};
    if(checkArgs(L,args)==1){
        int i = lua_tointeger(L,1);

        if(i<0&&i>models.size()-1){
            lua_pushstring(L,"Out of bounds");
            return 1;
        }

        LMesh neww = models[i];
        neww.name = lua_tostring(L,2);
        lua_pushinteger(L,models.size());
        models.push_back(neww);
        return 1;
    } else {
        lua_pushstring(L,"Wrong args");
        return 1;
    }
}

int ShaderCreate(lua_State *L){
    vector<string> args {"string","string"};
    string vs,fs,name;
    if(checkArgs(L,args)==1){
        string what = lua_tostring(L,1);
        vs = LoadShader(what + ".vs");
        fs = LoadShader(what + ".fs");
        name = lua_tostring(L,2);
    } else {
        vector<string> args2 {"string","string","string"};
        if(checkArgs(L,args2)==1){
            vs = lua_tostring(L,1);
            fs = lua_tostring(L,2);
            name = lua_tostring(L,3);
        } else {
            lua_pushstring(L,"Wrong args");
            return 1;
        }
    }
    LShader neww;
    neww.program = glCreateProgram();
    neww.shaders[0] = CreateShader(vs, GL_VERTEX_SHADER);
    neww.shaders[1] = CreateShader(fs, GL_FRAGMENT_SHADER);

    for(unsigned int i = 0; i < neww.shNum;i++)
        glAttachShader(neww.program,neww.shaders[i]);

    glBindAttribLocation(neww.program, 0, "position");
    glBindAttribLocation(neww.program, 1, "texCoord");
    glBindAttribLocation(neww.program, 2, "normal");

    glLinkProgram(neww.program);
    CheckShaderError(neww.program,GL_LINK_STATUS, true, "Error: Program failed to link: ");

    glValidateProgram(neww.program);
    CheckShaderError(neww.program,GL_VALIDATE_STATUS, true, "Error: Program is invalid: ");

    neww.uniforms[neww.TRANSFORM_U] = glGetUniformLocation(neww.program, "transform");
    neww.uniforms[neww.UIPOS_U] = glGetUniformLocation(neww.program, "uipos");
    neww.uniforms[neww.UISCL_U] = glGetUniformLocation(neww.program, "uiscl");
    neww.name = name;
    lua_pushinteger(L,shaders.size());
    shaders.push_back(neww);
    return 1;
}

int ShaderBind(lua_State *L){
    vector<string> args {"number"};
    if(checkArgs(L,args)==1){
        int i = lua_tointeger(L,1);

        if(i<0&&i>shaders.size()-1){
            lua_pushstring(L,"Out of bounds");
            return 1;
        }

        glUseProgram(shaders[i].program);
        return 0;
    } else {
        lua_pushstring(L,"Wrong args");
        return 1;
    }
}

int CameraCreate(lua_State *L){
    vector<string> args {"number","number","number","number","number","number","string"};
    if(checkArgs(L,args)==1){
        float aspect = (float)displ.width/(float)displ.height;
        cout << aspect << endl;
        CamObj neww(vec3((float)lua_tonumber(L,1),(float)lua_tonumber(L,2),(float)lua_tonumber(L,3)),3.14f/180*(float)lua_tonumber(L,4),aspect,(float)lua_tonumber(L,5),(float)lua_tonumber(L,6),lua_tostring(L,7));
        lua_pushinteger(L,cams.size());
        cams.push_back(neww);
        return 1;
    } else {
        lua_pushstring(L,"Wrong args");
        return 1;
    }
}

int CameraSetRotation(lua_State *L){
    vector<string> args {"number","number","number","number"};
    if(checkArgs(L,args)==1){
        int i = lua_tointeger(L,4);

        if(i<0&&i>cams.size()-1){
            lua_pushstring(L,"Out of bounds");
            return 1;
        }

        vec3 a((float)lua_tonumber(L,1),(float)lua_tonumber(L,2),(float)lua_tonumber(L,3));
        cams[i].cam.rotation = toQuat(orientate3(a));
        cams[i].cam.forw = cams[i].cam.rotation * vec3(0,0,1);
        cams[i].cam.up = cams[i].cam.rotation * vec3(0,1,0);
        cams[i].cam.left = cams[i].cam.rotation * vec3(-1,0,0);
        return 0;
    } else {
        lua_pushstring(L,"Wrong args");
        return 1;
    }
}

int CameraGetFwd(lua_State *L){
    vector<string> args {"number"};
    if(checkArgs(L,args)==1){
        int i = lua_tointeger(L,1);

        if(i<0&&i>cams.size()-1){
            lua_pushstring(L,"Out of bounds");
            return 1;
        }

        lua_pushnumber(L,cams[i].cam.forw.x);
        lua_pushnumber(L,cams[i].cam.forw.y);
        lua_pushnumber(L,cams[i].cam.forw.z);
        return 3;
    } else {
        lua_pushstring(L,"Wrong args");
        return 1;
    }
}

int CameraGetUp(lua_State *L){
    vector<string> args {"number"};
    if(checkArgs(L,args)==1){
        int i = lua_tointeger(L,1);

        if(i<0&&i>cams.size()-1){
            lua_pushstring(L,"Out of bounds");
            return 1;
        }

        lua_pushnumber(L,cams[i].cam.up.x);
        lua_pushnumber(L,cams[i].cam.up.y);
        lua_pushnumber(L,cams[i].cam.up.z);
        return 3;
    } else {
        lua_pushstring(L,"Wrong args");
        return 1;
    }
}

int CameraGetLeft(lua_State *L){
    vector<string> args {"number"};
    if(checkArgs(L,args)==1){
        int i = lua_tointeger(L,1);

        if(i<0&&i>cams.size()-1){
            lua_pushstring(L,"Out of bounds");
            return 1;
        }
        lua_pushnumber(L,cams[i].cam.left.x);
        lua_pushnumber(L,cams[i].cam.left.y);
        lua_pushnumber(L,cams[i].cam.left.z);
        return 3;
    } else {
        lua_pushstring(L,"Wrong args");
        return 1;
    }
}

int CameraSetPosition(lua_State *L){
    vector<string> args {"number","number","number","number"};
    if(checkArgs(L,args)==1){
        int i = lua_tointeger(L,4);

        if(i<0&&i>cams.size()-1){
            lua_pushstring(L,"Out of bounds");
            return 1;
        }

        cams[i].cam.getPosition()->x = (float)lua_tonumber(L,1);
        cams[i].cam.getPosition()->y = (float)lua_tonumber(L,2);
        cams[i].cam.getPosition()->z = (float)lua_tonumber(L,3);
        return 0;
    } else {
        lua_pushstring(L,"Wrong args");
        return 1;
    }
}

int CameraSetFOV(lua_State *L){
    vector<string> args {"number","number"};
    if(checkArgs(L,args)==1){
        int i = lua_tointeger(L,2);

        if(i<0&&i>cams.size()-1){
            lua_pushstring(L,"Out of bounds");
            return 1;
        }

        float ff = std::max(std::min((float)lua_tonumber(L,1),180.0f),0.0f);
        cams[i].cam.fov = ff;
        cams[i].cam.DoCamera(cams[i].cam.position,3.14f/180*ff,(float)displ.width/displ.height,cams[i].cam.zn,cams[i].cam.zf);
        return 0;
    } else {
        lua_pushstring(L,"Wrong args");
        return 1;
    }
}

int CameraSetZRange(lua_State *L){
    vector<string> args {"number","number","number"};
    if(checkArgs(L,args)==1){
        int i = lua_tointeger(L,3);

        if(i<0&&i>cams.size()-1){
            lua_pushstring(L,"Out of bounds");
            return 1;
        }

        float ff = (float)lua_tonumber(L,1);
        float ff2 = (float)lua_tonumber(L,2);
        cams[i].cam.zn = ff;
        cams[i].cam.zf = ff2;
        cams[i].cam.DoCamera(cams[i].cam.position,cams[i].cam.fov,(float)displ.width/displ.height,ff,ff2);
        return 0;
    } else {
        lua_pushstring(L,"Wrong args");
        return 1;
    }
}

int TransformCreate(lua_State *L){
    vector<string> args {"number","number","number","number","number","number","number","number","number","string"};
    if(checkArgs(L,args)==1){
        TfObj neww(vec3((float)lua_tonumber(L,1),(float)lua_tonumber(L,2),(float)lua_tonumber(L,3)), vec3((float)lua_tonumber(L,4),(float)lua_tonumber(L,5),(float)lua_tonumber(L,6)), vec3((float)lua_tonumber(L,7),(float)lua_tonumber(L,8),(float)lua_tonumber(L,9)),lua_tostring(L,10));
        lua_pushinteger(L,tfs.size());
        tfs.push_back(neww);
        return 1;
    } else {
        vector<string> arg2 {"string"};
        if(checkArgs(L,arg2)==1){
            TfObj neww(lua_tostring(L,1));
            lua_pushinteger(L,tfs.size());
            tfs.push_back(neww);
            return 1;
        } else {
            lua_pushstring(L,"Wrong args");
            return 1;
        }
    }
}

int TransformSetPos(lua_State *L){
    vector<string> args {"number","number","number","number"};
    if(checkArgs(L,args)==1){
        int i = lua_tointeger(L,4);

        if(i<0&&i>tfs.size()-1){
            lua_pushstring(L,"Out of bounds");
            return 1;
        }

        tfs[i].tf.SetPos(vec3((float)lua_tonumber(L,1),(float)lua_tonumber(L,2),(float)lua_tonumber(L,3)));
        return 0;
    } else {
        lua_pushstring(L,"Wrong args");
        return 1;
    }
}

int TransformSetRot(lua_State *L){
    vector<string> args {"number","number","number","number"};
    if(checkArgs(L,args)==1){
        int i = lua_tointeger(L,4);

        if(i<0&&i>tfs.size()-1){
            lua_pushstring(L,"Out of bounds");
            return 1;
        }
        tfs[i].tf.SetRot(vec3((float)lua_tonumber(L,1),(float)lua_tonumber(L,2),(float)lua_tonumber(L,3)));
        return 0;
    } else {
        lua_pushstring(L,"Wrong args");
        return 1;
    }
}

int TransformSetScale(lua_State *L){
    vector<string> args {"number","number","number","number"};
    if(checkArgs(L,args)==1){
        int i = lua_tointeger(L,4);

        if(i<0&&i>tfs.size()-1){
            lua_pushstring(L,"Out of bounds");
            return 1;
        }

        tfs[i].tf.SetScale(vec3((float)lua_tonumber(L,1),(float)lua_tonumber(L,2),(float)lua_tonumber(L,3)));
        return 0;
    } else {
        lua_pushstring(L,"Wrong args");
        return 1;
    }
}

int ShaderUpdate(lua_State *L){
    vector<string> args {"number","number","number"};
    if(checkArgs(L,args)==1){
        int i = lua_tointeger(L,1);

        if(i<0&&i>shaders.size()-1){
            lua_pushstring(L,"Out of bounds");
            return 1;
        }

        int j = lua_tointeger(L,2);

        if(j<0&&j>cams.size()-1){
            lua_pushstring(L,"Out of bounds");
            return 1;
        }

        int n = lua_tointeger(L,3);

        if(n<0&&n>tfs.size()-1){
            lua_pushstring(L,"Out of bounds");
            return 1;
        }

        glm::mat4 model = cams[j].cam.getViewProjection() * tfs[n].tf.GetModel();
        glUniformMatrix4fv(shaders[i].uniforms[shaders[i].TRANSFORM_U], 1, GL_FALSE, &model[0][0]);
        return 0;
    } else {
        lua_pushstring(L,"Wrong args");
        return 1;
    }
}

int ShaderUpdateUI(lua_State *L){
    vector<string> args {"number","number","number","number","number"};
    if(checkArgs(L,args)==1){
        int i = lua_tointeger(L,1);

        if(i<0&&i>shaders.size()-1){
            lua_pushstring(L,"Out of bounds");
            return 1;
        }

        int x = lua_tointeger(L,2), y = lua_tointeger(L,3), w = lua_tointeger(L,4), h = lua_tointeger(L,5);
        float fx = (float)2/displ.width*x, fy = (float)2/displ.height*y, fw = (float)1/displ.width*w, fh = (float)1/displ.height*h;
        fx=fx-1;
        fy=fy-1;
        fy=fy*(-1);
        glUniform2f(shaders[i].uniforms[shaders[i].UIPOS_U],fx,fy);
        glUniform2f(shaders[i].uniforms[shaders[i].UISCL_U],fw,fh);
        return 0;
    } else {
        lua_pushstring(L,"Wrong args");
        return 1;
    }
}

int DisplayClear(lua_State *L){
    vector<string> args {"number","number","number","number"};
    if(checkArgs(L,args)==1){
        displ.Clear((float)lua_tonumber(L,1),(float)lua_tonumber(L,2),(float)lua_tonumber(L,3),(float)lua_tonumber(L,4));
        return 0;
    } else {
        lua_pushstring(L,"Wrong args");
        return 1;
    }
}

int TextDraw(lua_State *L){
    vector<string> args {"string","number","number"};
    if(checkArgs(L,args)==1){
        glMatrixMode(GL_PROJECTION);
        string text = lua_tostring(L,1);
        int x = lua_tointeger(L,2);
        int y = lua_tointeger(L,3);
        int scrw = displ.width;
        int scrh = displ.height;

        double *matrix = new double[16];
        glGetDoublev(GL_PROJECTION_MATRIX,matrix);
        glLoadIdentity();
        glOrtho(0,scrw,scrh,0,-5,5);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glPushMatrix();
        glLoadIdentity();
        glRasterPos2i(x,y);
        for(int i=0;i<text.length();i++){
            glutBitmapCharacter(GLUT_BITMAP_9_BY_15, (int)text.c_str()[i]);
        }
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
    } else {
        lua_pushstring(L,"Wrong args");
        return 1;
    }
}

int TextureCreate(lua_State *L){
    vector<string> args {"string","string"};
    if(checkArgs(L,args)==1){
        LTexture neww;
        neww.name = lua_tostring(L,2);
        int index = textures.size();
        lua_pushinteger(L,textures.size());
        textures.push_back(neww);
        LTextureLoad(lua_tostring(L,1),index);
        return 1;
    } else {
        lua_pushstring(L,"Wrong args");
        return 1;
    }
}

int TextureBind(lua_State *L){
    vector<string> args {"number","number"};
    if(checkArgs(L,args)==1){
        int i = lua_tointeger(L,1);

        if(i<0&&i>textures.size()-1){
            lua_pushstring(L,"Out of bounds");
            return 1;
        }

        int unit = lua_tointeger(L,2);
        assert(unit >= 0 && unit <= 31);
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, textures[i].texture);
        return 0;
    } else {
        lua_pushstring(L,"Wrong args");
        return 1;
    }
}


int UniformF(lua_State *L){
    vector<string> a1 {"number","string"};
    if(checkArgs(L,a1)==1){
        GLint id;
        glGetIntegerv(GL_CURRENT_PROGRAM,&id);
        glUniform1f(glGetUniformLocation(id,lua_tostring(L,2)),(float)lua_tonumber(L,1));
        return 0;
    } else {
        vector<string> a2 {"number","number","string"};
        if(checkArgs(L,a2)==1){
            GLint id;
            glGetIntegerv(GL_CURRENT_PROGRAM,&id);
            glUniform2f(glGetUniformLocation(id,lua_tostring(L,3)),(float)lua_tonumber(L,1),(float)lua_tonumber(L,2));
            return 0;
        } else {
            vector<string> a3 {"number","number","number","string"};
            if(checkArgs(L,a3)==1){
                GLint id;
                glGetIntegerv(GL_CURRENT_PROGRAM,&id);
                glUniform3f(glGetUniformLocation(id,lua_tostring(L,4)),(float)lua_tonumber(L,1),(float)lua_tonumber(L,2),(float)lua_tonumber(L,3));
                return 0;
            } else {
                vector<string> a4 {"number","number","number","number","string"};
                if(checkArgs(L,a4)==1){
                    GLint id;
                    glGetIntegerv(GL_CURRENT_PROGRAM,&id);
                    glUniform4f(glGetUniformLocation(id,lua_tostring(L,5)),(float)lua_tonumber(L,1),(float)lua_tonumber(L,2),(float)lua_tonumber(L,3),(float)lua_tonumber(L,4));
                    return 0;
                } else {
                    lua_pushstring(L,"Wrong args");
                    return 1;
                }
            }
        }
    }
}

int UniformI(lua_State *L){
    vector<string> a1 {"number","string"};
    if(checkArgs(L,a1)==1){
        GLint id;
        glGetIntegerv(GL_CURRENT_PROGRAM,&id);
        glUniform1i(glGetUniformLocation(id,lua_tostring(L,2)),lua_tointeger(L,1));
        return 0;
    } else {
        vector<string> a2 {"number","number","string"};
        if(checkArgs(L,a2)==1){
            GLint id;
            glGetIntegerv(GL_CURRENT_PROGRAM,&id);
            glUniform2i(glGetUniformLocation(id,lua_tostring(L,3)),lua_tointeger(L,1),lua_tointeger(L,2));
            return 0;
        } else {
            vector<string> a3 {"number","number","number","string"};
            if(checkArgs(L,a3)==1){
                GLint id;
                glGetIntegerv(GL_CURRENT_PROGRAM,&id);
                glUniform3i(glGetUniformLocation(id,lua_tostring(L,4)),lua_tointeger(L,1),lua_tointeger(L,2),lua_tointeger(L,3));
                return 0;
            } else {
                vector<string> a4 {"number","number","number","number","string"};
                if(checkArgs(L,a4)==1){
                    GLint id;
                    glGetIntegerv(GL_CURRENT_PROGRAM,&id);
                    glUniform4i(glGetUniformLocation(id,lua_tostring(L,5)),lua_tointeger(L,1),lua_tointeger(L,2),lua_tointeger(L,3),lua_tointeger(L,4));
                    return 0;
                } else {
                    lua_pushstring(L,"Wrong args");
                    return 1;
                }
            }
        }
    }
}

int VecToQuat(lua_State *L){
    vector<string> args {"number","number","number"};
    if(checkArgs(L,args)==1){
        quat a = toQuat(orientate3(vec3(lua_tonumber(L,1),lua_tonumber(L,2),lua_tonumber(L,3))));
        lua_pushnumber(L,a.w);
        lua_pushnumber(L,a.x);
        lua_pushnumber(L,a.y);
        lua_pushnumber(L,a.z);
        return 4;
    } else {
        lua_pushstring(L,"Wrong args");
        return 1;
    }
}

int QuatToVec(lua_State *L){
    vector<string> args {"number","number","number","number"};
    if(checkArgs(L,args)==1){
        quat q(lua_tonumber(L,1),lua_tonumber(L,2),lua_tonumber(L,3),lua_tonumber(L,4));
        vec3 a = eulerAngles(q);

        lua_pushnumber(L,a.x);
        lua_pushnumber(L,a.y);
        lua_pushnumber(L,a.z);
        return 3;
    } else {
        lua_pushstring(L,"Wrong args");
        return 1;
    }
}

int GetName(lua_State *L){
    vector<string> args {"number","number"};
    if(checkArgs(L,args)==1){
        int i = lua_tointeger(L,1);
        int what = lua_tointeger(L,2);

        if(what == 1){ // Shader
            if(i<0&&i>shaders.size()-1){
                lua_pushstring(L,"Out of bounds");
                return 1;
            }
            lua_pushstring(L,shaders[i].name.c_str());
            return 1;
        } else if(what == 2){ // Mesh
            if(i<0&&i>models.size()-1){
                lua_pushstring(L,"Out of bounds");
                return 1;
            }
            lua_pushstring(L,models[i].name.c_str());
            return 1;
        } else if(what == 3){ // Texture
            if(i<0&&i>textures.size()-1){
                lua_pushstring(L,"Out of bounds");
                return 1;
            }
            lua_pushstring(L,textures[i].name.c_str());
            return 1;
        } else if(what == 4){ // Camera
            if(i<0&&i>cams.size()-1){
                lua_pushstring(L,"Out of bounds");
                return 1;
            }
            lua_pushstring(L,cams[i].name.c_str());
            return 1;
        } else if(what == 5){ // Transform
            if(i<0&&i>tfs.size()-1){
                lua_pushstring(L,"Out of bounds");
                return 1;
            }
            lua_pushstring(L,tfs[i].name.c_str());
            return 1;
        } else if(what == 6){ // Transform
            if(i<0&&i>paths.size()-1){
                lua_pushstring(L,"Out of bounds");
                return 1;
            }
            lua_pushstring(L,paths[i].name.c_str());
            return 1;
        }

    } else {
        lua_pushstring(L,"Wrong args");
        return 1;
    }
}


int GetID(lua_State *L){
    vector<string> args {"string","number"};
    if(checkArgs(L,args)==1){
        int id = -1;
        string nam = lua_tostring(L,1);
        int what = lua_tointeger(L,2);

        if(what == 1){ // Shader
            for(int i=0;i<shaders.size();i++){
                if(shaders[i].name == nam){
                    id = i;
                    break;
                }
            }
        } else if(what == 2){ // Mesh
            for(int i=0;i<models.size();i++){
                if(models[i].name == nam){
                    id = i;
                    break;
                }
            }
        } else if(what == 3){ // Texture
            for(int i=0;i<textures.size();i++){
                if(textures[i].name == nam){
                    id = i;
                    break;
                }
            }
        } else if(what == 4){ // Camera
            for(int i=0;i<cams.size();i++){
                if(cams[i].name == nam){
                    id = i;
                    break;
                }
            }
        } else if(what == 5){ // Transform
            for(int i=0;i<tfs.size();i++){
                if(tfs[i].name == nam){
                    id = i;
                    break;
                }
            }
        } else if(what == 6){ // Transform
            for(int i=0;i<paths.size();i++){
                if(paths[i].name == nam){
                    id = i;
                    break;
                }
            }
        }

        lua_pushinteger(L,id);
        return 1;

    } else {
        lua_pushstring(L,"Wrong args");
        return 1;
    }
}

int DeleteObje(lua_State *L){
    vector<string> args {"number","number"};
    if(checkArgs(L,args)==1){
        int nam = lua_tointeger(L,1);
        int what = lua_tointeger(L,2);

        if(what == 1){ // Shader
            if(nam<0&&nam>shaders.size()){
                lua_pushstring(L,"Out of bounds");
                return 1;
            }
            for(unsigned int i = 0; i < shaders[nam].shNum;i++){
                glDetachShader(shaders[nam].program,shaders[nam].shaders[i]);
                glDeleteShader(shaders[nam].shaders[i]);
            }
            glDeleteProgram(shaders[nam].program);
            shaders.erase(shaders.begin()+(nam));
        } else if(what == 2){ // Mesh
            if(nam<0&&nam>models.size()){
                lua_pushstring(L,"Out of bounds");
                return 1;
            }
            glDeleteVertexArrays(1,&models[nam].vertexArrObj);
            models.erase(models.begin()+nam);
        } else if(what == 3){ // Texture
            if(nam<0&&nam>textures.size()){
                lua_pushstring(L,"Out of bounds");
                return 1;
            }
            glDeleteTextures(1,&textures[nam].texture);
            textures.erase(textures.begin()+nam);
        } else if(what == 4){ // Camera
            if(nam<0&&nam>cams.size()){
                lua_pushstring(L,"Out of bounds");
                return 1;
            }
            cams.erase(cams.begin()+nam);
        } else if(what == 5){ // Transform
            if(nam<0&&nam>tfs.size()){
                lua_pushstring(L,"Out of bounds");
                return 1;
            }
            tfs.erase(tfs.begin()+nam);
        } else if(what == 6){ // Transform
            if(nam<0&&nam>paths.size()){
                lua_pushstring(L,"Out of bounds");
                return 1;
            }
            paths.erase(paths.begin()+nam);
        }

    } else {
        lua_pushstring(L,"Wrong args");
        return 1;
    }
}

int PathCreate(lua_State *L){
    vector<string> args {"number","number","string"};
    if(checkArgs(L,args)==1){
        LPath a;
        int ind = paths.size();
        paths.push_back(a);
        lua_pushinteger(L,ind);
        AStar::Vec2i si;
        si.x = lua_tointeger(L,1);
        si.y = lua_tointeger(L,2);
        paths[ind].gen.setWorldSize(si);
        paths[ind].name = lua_tostring(L,3);
        paths[ind].w = si.x;
        paths[ind].h = si.y;
        return 1;
    } else {
        lua_pushstring(L,"Wrong args");
        return 1;
    }
}

int PathSetCollision(lua_State *L){
    vector<string> args {"number","number","number","boolean"};
    if(checkArgs(L,args)==1){
        int i = lua_tointeger(L,1);

        if(i<-1&&i>paths.size()-1){
            lua_pushstring(L,"Out of bounds");
            return 1;
        }

        int x = lua_tointeger(L,2);
        int y = lua_tointeger(L,3);
        bool b = lua_toboolean(L,4);

        AStar::Vec2i point;
        point.x = x-1;
        point.y = y-1;

        if(b){
            paths[i].gen.addCollision(point);
        } else {
            paths[i].gen.removeCollision(point);
        }

    } else {
        lua_pushstring(L,"Wrong args");
        return 1;
    }
}

int PathFind(lua_State *L){
    vector<string> args {"number","number","number","number","number"};
    if(checkArgs(L,args)==1){
        int i = lua_tointeger(L,1);

        if(i<0&&i>paths.size()-1){
            lua_pushstring(L,"Out of bounds");
            return 1;
        }

        int x = lua_tointeger(L,2);
        int y = lua_tointeger(L,3);
        int wx = lua_tointeger(L,4);
        int wy = lua_tointeger(L,5);

        AStar::Vec2i point;
        point.x = x;
        point.y = y;
        AStar::Vec2i endpoint;
        endpoint.x = wx;
        endpoint.y = wy;

        AStar::CoordinateList f = paths[i].gen.findPath(point,endpoint);

        lua_newtable(L);

        for(int j=0;j<f.size();j++){
            AStar::Vec2i tp = f[j];
            lua_newtable(L);
            lua_pushinteger(L,tp.x);
            lua_settable(L,-2);
            lua_pushinteger(L,tp.y);
            lua_settable(L,-2);
            lua_settable(L,-2);
        }
        return 1;
    } else {
        lua_pushstring(L,"Wrong args");
        return 1;
    }
}

int SetWireframe(lua_State *L){
    vector<string> args {"boolean"};
    if(checkArgs(L,args)==1){
        bool wireframe = lua_toboolean(L,1);
        if(wireframe)
            glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
        else
            glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
    } else {
        lua_pushstring(L,"Wrong args");
        return 1;
    }
}


int Debug(lua_State *L){
    cout << "==== MODELS ====" << endl;
    for(int i=0;i<models.size();i++){
        cout << models[i].name << ": " << endl;
    }

    cout << "==== Cameras ====" << endl;
    for(int i=0;i<cams.size();i++){
        cout << cams[i].name << ": " << cams[i].cam.getPosition()->x << "-" << cams[i].cam.getPosition()->y << "-" << cams[i].cam.getPosition()->z << " = " << cams[i].cam.getForward()->x << "-" << cams[i].cam.getForward()->y << "- " << cams[i].cam.getForward()->z << endl;
    }

    cout << "==== Transforms ====" << endl;
    for(int i=0;i<tfs.size();i++){
        cout << tfs[i].name << ": " << tfs[i].tf.GetPos()->x << "-" << tfs[i].tf.GetPos()->y << "-" << tfs[i].tf.GetPos()->z << " = " << tfs[i].tf.GetRot()->x << "-" << tfs[i].tf.GetRot()->y << "-" << tfs[i].tf.GetRot()->z << " = " << tfs[i].tf.GetScale()->x << "-" << tfs[i].tf.GetScale()->y << "-" << tfs[i].tf.GetScale()->z << endl;
    }
}

int main(int argc, char** argv)
{
    addLuaFunc("pullEvent",pullEvent);
    addLuaFunc("isDone",isDone);
    addLuaFunc("initWindow",initWindow);
    addLuaFunc("sleep",sleep);
    addLuaFunc("setLuaKiller",setLuaKiller);

    addLuaFunc("svOpenSlot",svOpenSlot);
    addLuaFunc("svCloseSlot",svCloseSlot);
    addLuaFunc("svLockSlot",svLockSlot);
    addLuaFunc("svUnlockSlot",svUnlockSlot);
    addLuaFunc("svGetSampleType",svGetSampleType);
    addLuaFunc("svLoad",svLoad);
    addLuaFunc("svPlay",svPlay);
    addLuaFunc("svPlayFromBeginning",svPlayFromBeginning);
    addLuaFunc("svStop",svStop);
    addLuaFunc("svSetAutostop",svSetAutostop);
    addLuaFunc("svEndOfSong",svEndOfSong);
    addLuaFunc("svRewind",svRewind);
    addLuaFunc("svVolume",svVolume);
    addLuaFunc("svSendEvent",svSendEvent);
    addLuaFunc("svGetCurrentLine",svGetCurrentLine);
    addLuaFunc("svGetCurrentLine2",svGetCurrentLine2);
    addLuaFunc("svGetCurrentSignalLevel",svGetCurrentSignalLevel);
    addLuaFunc("svGetSongName",svGetSongName);
    addLuaFunc("svGetSongBPM",svGetSongBPM);
    addLuaFunc("svGetSongTPL",svGetSongTPL);
    addLuaFunc("svGetSongLengthFrames",svGetSongLengthFrames);
    addLuaFunc("svGetSongLengthLines",svGetSongLengthLines);
    addLuaFunc("svNewModule",svNewModule);
    addLuaFunc("svRemoveModule",svRemoveModule);
    addLuaFunc("svConnectModule",svConnectModule);
    addLuaFunc("svDisconnectModule",svDisconnectModule);
    addLuaFunc("svLoadModule",svLoadModule);
    addLuaFunc("svSamplerLoad",svSamplerLoad);
    addLuaFunc("svGetNumberOfModules",svGetNumberOfModules);
    addLuaFunc("svGetModuleFlags",svGetModuleFlags);
    addLuaFunc("svGetModuleInputs",svGetModuleInputs);
    addLuaFunc("svGetModuleOutputs",svGetModuleOutputs);
    addLuaFunc("svGetModuleName",svGetModuleName);
    addLuaFunc("svGetModuleXY",svGetModuleXY);
    addLuaFunc("svGetModuleColor",svGetModuleColor);
    addLuaFunc("svGetNumberOfModuleCtls",svGetNumberOfModuleCtls);
    addLuaFunc("svGetModuleCtlName",svGetModuleCtlName);
    addLuaFunc("svGetModuleCtlValue",svGetModuleCtlValue);
    addLuaFunc("svGetNumberOfPatterns",svGetNumberOfPatterns);
    addLuaFunc("svGetPatternX",svGetPatternX);
    addLuaFunc("svGetPatternY",svGetPatternY);
    addLuaFunc("svGetPatternTracks",svGetPatternTracks);
    addLuaFunc("svGetPatternLines",svGetPatternLines);
    addLuaFunc("svPatternMute",svPatternMute);
    addLuaFunc("svGetTicks",svGetTicks);
    addLuaFunc("svGetTicksPerSecond",svGetTicksPerSecond);

    addLuaFunc("MeshCreate",MeshCreate);
    addLuaFunc("MeshDraw",MeshDraw);
    addLuaFunc("MeshCopy",MeshCopy);
    addLuaFunc("ShaderCreate",ShaderCreate);
    addLuaFunc("ShaderBind",ShaderBind);
    addLuaFunc("CameraCreate",CameraCreate);
    addLuaFunc("CameraSetRotation",CameraSetRotation);
    addLuaFunc("CameraSetPosition",CameraSetPosition);
    addLuaFunc("CameraSetFOV",CameraSetFOV);
    addLuaFunc("CameraSetZRange",CameraSetZRange);
    addLuaFunc("CameraGetFwd",CameraGetFwd);
    addLuaFunc("CameraGetUp",CameraGetUp);
    addLuaFunc("CameraGetLeft",CameraGetLeft);
    addLuaFunc("TransformCreate",TransformCreate);
    addLuaFunc("TransformSetPos",TransformSetPos);
    addLuaFunc("TransformSetRot",TransformSetRot);
    addLuaFunc("TransformSetScale",TransformSetScale);
    addLuaFunc("ShaderUpdate",ShaderUpdate);
    addLuaFunc("ShaderUpdateUI",ShaderUpdateUI);
    addLuaFunc("DisplayClear",DisplayClear);
    addLuaFunc("TextDraw",TextDraw);
    addLuaFunc("TextureCreate",TextureCreate);
    addLuaFunc("TextureBind",TextureBind);
    addLuaFunc("isKeyDown",isKeyDown);
    addLuaFunc("getMousePos",getMousePos);
    addLuaFunc("getMouseVel",getMouseVel);
    addLuaFunc("setMousePos",setMousePos);
    addLuaFunc("showMouse",showMouse);
    addLuaFunc("getKeyEvent",getKeyEvent);
    addLuaFunc("GetID",GetID);
    addLuaFunc("GetName",GetName);
    addLuaFunc("DeleteObject",DeleteObje);
    addLuaFunc("PathCreate",PathCreate);
    addLuaFunc("PathSetCollision",PathSetCollision);
    addLuaFunc("PathFind",PathFind);
    addLuaFunc("SetWireframe",SetWireframe);
    addLuaFunc("runThread",runThreat);
    addLuaFunc("send",send);

    addLuaFunc("UniformF",UniformF);
    addLuaFunc("UniformI",UniformI);
    addLuaFunc("VecToQuat",VecToQuat);
    addLuaFunc("QuatToVec",QuatToVec);

    addLuaFunc("Debug",Debug);

    if( sv_load_dll() )
	return 1;

	int ver = sv_init( 0, 44100, 2, 0 );
    if( ver >= 0 )
    {
	int major = ( ver >> 16 ) & 255;
	int minor1 = ( ver >> 8 ) & 255;
	int minor2 = ( ver ) & 255;
	printf( "SunVox lib version: %d.%d.%d\n", major, minor1, minor2 );

    ManyMouse_Init();


    #ifdef RELEASETHING
    ShowWindow(GetConsoleWindow(),SW_HIDE);
    #endif // RELEASETHING
    execute(mainLUA.c_str());
    executing = false;
    /*
    cout << "\n\nDeleting all shaders and programs:\n";
    for(int i=0;i<shaders.size();i++){
        cout << shaders[i].name << endl;
        for(unsigned int j = 0; j < shaders[i].shNum;j++){
            glDetachShader(shaders[i].program,shaders[i].shaders[j]);
            glDeleteShader(shaders[i].shaders[j]);
        }
        glDeleteProgram(shaders[i].program);
    }

    cout << "\n\nDeleting all meshes:\n";

    for(int i=0;i<models.size();i++){
        cout << models[i].name << endl;
        glDeleteVertexArrays(1,&models[i].vertexArrObj);
    }

    cout << "\n\nDeleting all textures:\n";
    for(int i=0;i<textures.size();i++){
        cout << textures[i].name << endl;
        glDeleteTextures(1, &textures[i].texture);
    }
    cout << endl;

    cout << "\n\nAll transforms that was here:\n";
    for(int i=0;i<tfs.size();i++){
        cout << tfs[i].name << endl;
    }
    cout << endl;*/
    sv_stop( 0 );

	sv_close_slot( 0 );

	sv_deinit();
    }
    else
    {
	printf( "sv_init() error %d\n", ver );
    }

    sv_unload_dll();

    ManyMouse_Quit();
    SDL_Quit();
}
