#ifndef MESH_H
#define MESH_H
#include <string>
#include "glm/glm.hpp"
#include "GL/glew.h"
#include "obj_loader.h"

using namespace std;

struct LLVertex{
    LLVertex(const glm::vec3& pos, const glm::vec2& texCoord, const glm::vec3& normal)
	{
		this->pos = pos;
		texPos = texCoord;
		this->normal = normal;
    }
    LLVertex(){

    }
    glm::vec3 pos;
    glm::vec2 texPos;
    glm::vec3 normal;
};

void CheckShaderError(GLuint shader, GLuint flag, bool isProgram, const string& errorMessage)
{
    GLint success = 0;
    GLchar error[1024] = { 0 };

    if(isProgram)
        glGetProgramiv(shader, flag, &success);
    else
        glGetShaderiv(shader, flag, &success);

    if(success == GL_FALSE)
    {
        if(isProgram)
            glGetProgramInfoLog(shader, sizeof(error), NULL, error);
        else
            glGetShaderInfoLog(shader, sizeof(error), NULL, error);

        cerr << errorMessage << ": '" << error << "'" << endl;
    }
}

string LoadShader(const string& fileName)
{
    ifstream file;
    file.open((fileName).c_str());

    string output;
    string line;

    if(file.is_open())
    {
        while(file.good())
        {
            getline(file, line);
			output.append(line + "\n");
        }
    }
    else
    {
		cerr << "Unable to load shader: " << fileName << endl;
    }

    return output;
}

GLuint CreateShader(const string& text, GLenum shaderType){

    GLuint shader = glCreateShader(shaderType);

    if(shader == 0)
        cerr << "Error: Shader creation failed" << endl;

    const GLchar* shdr[1];
    shdr[0] = text.c_str();
    GLint shdrlen[1];
    shdrlen[0] = text.length();

    glShaderSource(shader, 1, shdr, shdrlen);
    glCompileShader(shader);

    CheckShaderError(shader, GL_COMPILE_STATUS, false, "Error: Shader failed to compile: ");

    return shader;
}

#endif // MESH_H
