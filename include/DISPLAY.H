#ifndef DISPLAY_H
#define DISPLAY_H

#include "SDL2/SDL.h"
#include "GL/glew.h"
#include <string>
#include <iostream>
#include "manymouse.h"

using namespace std;

class Display
{
public:
    Display(){

    }
    Display(int width, int height, const string& title, bool cull){
        if(SDL_Init(SDL_INIT_EVERYTHING)!=0){
            cerr << "SDL Failed to init: " << SDL_GetError() << endl;
        }

        SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

        this->width = width;
        this->height = height;

        wind = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL);
        glContext = SDL_GL_CreateContext(wind);

        GLenum status = glewInit();

        if(status != GLEW_OK){
            cerr << "GLEW is failed";
        }
        if(cull){
            glEnable(GL_DEPTH_TEST);

            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
        }
        ManyMouse_Init();
    };

    void Init(int width, int height, const string& title, bool cull){
        if(SDL_Init(SDL_INIT_EVERYTHING)!=0){
            cerr << "SDL Failed to init: " << SDL_GetError() << endl;
        }

        SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);


        this->width = width;
        this->height = height;

        wind = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL);
        glContext = SDL_GL_CreateContext(wind);

        GLenum status = glewInit();
        if(status != GLEW_OK){
            cerr << "GLEW is failed";
        }
        if(cull){
            glEnable(GL_DEPTH_TEST);

            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
        }
        ManyMouse_Init();
    }

    virtual ~Display(){
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(wind);
        SDL_Quit();
    };

    void Clear(float r, float g, float b, float a){
        glClearColor(r,g,b,a);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    }

    void Update(){
        SDL_GL_SwapWindow(wind);
    };

    void EventsP(){
        SDL_Event e;
        while(SDL_PollEvent(&e)){
            if(e.type == SDL_QUIT){
                done = true;
                break;
            }
        }
    }

    void setDone(bool d){
        done = d;
    }
    int width,height;

    bool focused;

    bool isDone(){
        return done;
    }
    SDL_Window* wind;

private:

    Display(const Display& ot){}
    Display& operator*(const Display& ot){}

    SDL_GLContext glContext;
    bool done = false;
};





#endif // DISPLAY_H
