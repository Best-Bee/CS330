///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

void SceneManager::LoadSceneTextures()
{
	bool bReturn = false;

	//bReturn = CreateGLTexture("stone_rough_light_grey.jpg", "stoneLight");
	//bReturn = CreateGLTexture("stone_rough_dark_metallic.jpg", "stoneDark");
	bReturn = CreateGLTexture("wood_natural.jpg", "woodReal");
	bReturn = CreateGLTexture("wood_planks_light_brown.jpg", "woodPlanks");
	//bReturn = CreateGLTexture("wood_planks_light_brown_two.jpg", "woodPlanksTwo");
	bReturn = CreateGLTexture("plastic_white.jpg", "plastic");
	bReturn = CreateGLTexture("Label.jpg", "prescription");
	bReturn = CreateGLTexture("rug_proper.png", "rug");
	bReturn = CreateGLTexture("cloth_yellow.jpg", "cloth");
	bReturn = CreateGLTexture("black_wood.jpg", "table");
	bReturn = CreateGLTexture("wallpaper_beige.jpg", "wall");
	bReturn = CreateGLTexture("black.jpg", "screen");
	bReturn = CreateGLTexture("pearlescent.jpg", "pencil");

	BindGLTextures();
}

/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used for configuring the various material
 *  settings for all of the objects within the 3D scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	/*** STUDENTS - add the code BELOW for defining object materials. ***/
	/*** There is no limit to the number of object materials that can ***/
	/*** be defined. Refer to the code in the OpenGL Sample for help  ***/


	OBJECT_MATERIAL woodMaterial;
	woodMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.ambientStrength = 0.2f;
	woodMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	woodMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.shininess = 0.3;
	woodMaterial.tag = "wood";
	m_objectMaterials.push_back(woodMaterial);

	OBJECT_MATERIAL tableMaterial;
	tableMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	tableMaterial.ambientStrength = 0.3f;
	tableMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.4f);
	tableMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);
	tableMaterial.shininess = 0.5;
	tableMaterial.tag = "table";
	m_objectMaterials.push_back(tableMaterial);

	OBJECT_MATERIAL rugMaterial;
	rugMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	rugMaterial.ambientStrength = 0.3f;
	rugMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	rugMaterial.specularColor = glm::vec3(0.3f, 0.3f, 0.3f);
	rugMaterial.shininess = 0.5;
	rugMaterial.tag = "rug";
	m_objectMaterials.push_back(rugMaterial);

	OBJECT_MATERIAL glassMaterial;
	glassMaterial.ambientColor = glm::vec3(0.5f, 0.5f, 0.5f);
	glassMaterial.ambientStrength = 0.3f;
	glassMaterial.diffuseColor = glm::vec3(0.8f, 0.8f, 0.8f);
	glassMaterial.specularColor = glm::vec3(0.8f, 0.8f, 0.8f);
	glassMaterial.shininess = 100.0;
	glassMaterial.tag = "glass";
	m_objectMaterials.push_back(glassMaterial);

}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// this line of code is NEEDED for telling the shaders to render 
	// the 3D scene with custom lighting, if no light sources have
	// been added then the display window will be black - to use the 
	// default OpenGL lighting then comment out the following line
	//m_pShaderManager->setBoolValue(g_UseLightingName, true);

	/*** STUDENTS - add the code BELOW for setting up light sources ***/
	/*** Up to four light sources can be defined. Refer to the code ***/
	/*** in the OpenGL Sample for help                              ***/

	//Sample fireplace lighting
	//m_pShaderManager->setVec3Value ("lightSources[0].position", -60.0f, 3.0f, 4.0f);
	//m_pShaderManager->setVec3Value ("lightSources[0].ambientColor", 0.80f, 0.30f, 0.30f);
	//m_pShaderManager->setVec3Value ("lightSources[0].diffuseColor", 0.6f, 0.3f, 0.3f);
	//m_pShaderManager->setVec3Value ("lightSources[0].specularColor", 0.2f, 0.0f, 0.0f);
	//m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 32.0f);
	//m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.05f);
	//m_pShaderManager->setFloatValue("lightSources[0].constant", 1.0f);
	//m_pShaderManager->setFloatValue("lightSources[0].linear", 0.01f);
	//m_pShaderManager->setFloatValue("lightSources[0].quadratic", 0.00f);

	m_pShaderManager->setVec3Value("lightSources[0].position", -4.0f, 2.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 0.0f);				//light turned off currently
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 1.00f);
	m_pShaderManager->setFloatValue("lightSources[0].constant", 1.0f);
	m_pShaderManager->setFloatValue("lightSources[0].linear", 0.00001f);
	m_pShaderManager->setFloatValue("lightSources[0].quadratic", 0.0f);

	m_pShaderManager->setVec3Value ("lightSources[1].position", 50.0f, 15.0f, 0.0f);
	m_pShaderManager->setVec3Value ("lightSources[1].ambientColor", 0.50f, 0.30f, 0.30f);
	m_pShaderManager->setVec3Value ("lightSources[1].diffuseColor", 0.3f, 0.3f, 0.3f);
	m_pShaderManager->setVec3Value ("lightSources[1].specularColor", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 16.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.03f);
	m_pShaderManager->setFloatValue("lightSources[1].constant", 1.0f);
	m_pShaderManager->setFloatValue("lightSources[1].linear", 0.01f);
	m_pShaderManager->setFloatValue("lightSources[1].quadratic", 0.00f);

	m_pShaderManager->setVec3Value ("lightSources[2].position", 15.0f, 8.0f, 15.0f);
	m_pShaderManager->setVec3Value ("lightSources[2].ambientColor", 0.60f, 0.20f, 0.60f);
	m_pShaderManager->setVec3Value ("lightSources[2].diffuseColor", 0.3f, 0.1f, 0.3f);
	m_pShaderManager->setVec3Value ("lightSources[2].specularColor", 0.0f, 0.0f, 0.1f);
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 6.0f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.05f);
	m_pShaderManager->setFloatValue("lightSources[2].constant", 1.0f);
	m_pShaderManager->setFloatValue("lightSources[2].linear", 0.01f);
	m_pShaderManager->setFloatValue("lightSources[2].quadratic", 0.00f);

	m_pShaderManager->setVec3Value ("lightSources[3].position", -10.0f, 8.0f, 15.0f);
	m_pShaderManager->setVec3Value ("lightSources[3].ambientColor", 0.10f, 0.10f, 0.10f);
	m_pShaderManager->setVec3Value ("lightSources[3].diffuseColor", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setVec3Value ("lightSources[3].specularColor", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setFloatValue("lightSources[3].focalStrength", 4.0f);
	m_pShaderManager->setFloatValue("lightSources[3].specularIntensity", 0.05f);
	m_pShaderManager->setFloatValue("lightSources[3].constant", 1.0f);
	m_pShaderManager->setFloatValue("lightSources[3].linear", 0.001f);
	m_pShaderManager->setFloatValue("lightSources[3].quadratic", 0.00f);

	m_pShaderManager->setBoolValue("bUseLighting", true);

}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// load the textures for the 3D scene
	LoadSceneTextures();

	// define the materials for objects in the scene
	DefineObjectMaterials();

	// add and define the light sources for the scene
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadConeMesh();

}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/

	/****************************************************************/

	// Floor

	scaleXYZ = glm::vec3(40.0f, 1.0f, 20.0f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(25.0f, 0.0f, 0.0f);

	SetTextureUVScale(10.0f, 10.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.5, 0.5, 0.5, 0.5);

	SetShaderTexture("woodPlanks");
	SetShaderMaterial("wood");

	m_basicMeshes->DrawPlaneMesh();

	/****************************************************************/

	// Walls

	// East Wall

	scaleXYZ = glm::vec3(8.0f, 10.0f, 40.0f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 90.0f;

	positionXYZ = glm::vec3(25.0f, 8.0f, -20.0f);

	SetTextureUVScale(10.0f, 10.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.5, 0.5, 0.5, 0.5);

	SetShaderTexture("wall");
	SetShaderMaterial("rug");

	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

	// Rugs

	// Rug 1

	// Rug base

	scaleXYZ = glm::vec3(10.0f, 0.1f, 20.0f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-4.0f, 0.1f, 0.0f);

	SetTextureUVScale(1.0f, 1.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("rug");
	SetShaderMaterial("rug");

	m_basicMeshes->DrawBoxMesh();

	// Rug border

	scaleXYZ = glm::vec3(10.5f, 0.1f, 20.5f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-4.0f, 0.09f, 0.0f);

	SetTextureUVScale(1.0f, 1.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("cloth");
	SetShaderMaterial("rug");
	
	m_basicMeshes->DrawBoxMesh();

	/*******************************/

	// Rug 2

	// Rug base

	scaleXYZ = glm::vec3(12.0f, 0.1f, 24.0f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(20.0f, 0.1f, -3.0f);

	SetTextureUVScale(1.0f, 1.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("rug");
	SetShaderMaterial("rug");

	m_basicMeshes->DrawBoxMesh();

	// Rug border

	scaleXYZ = glm::vec3(12.5f, 0.1f, 24.5f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(20.0f, 0.09f, -3.0f);

	SetTextureUVScale(1.0f, 1.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("cloth");
	SetShaderMaterial("rug");

	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/

	//MTG box
	scaleXYZ = glm::vec3(0.75f, 1.0f, 2.0f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-6.0f, 2.0f, 3.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 0, 1, 1);

	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	//Prescription 1 (S): 

	//Orange base
	scaleXYZ = glm::vec3(0.2f, 0.8f, 0.2f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-5.8f, 2.501f, 3.25f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 0.8, 0.5, 0.5);

	m_basicMeshes->DrawCylinderMesh();

	//Paper label
	scaleXYZ = glm::vec3(0.21f, 0.4f, 0.21f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-5.8f, 2.7f, 3.25f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);

	SetShaderTexture("prescription");

	m_basicMeshes->DrawCylinderMesh();

	//Lid bottom ring
	scaleXYZ = glm::vec3(0.26f, 0.03f, 0.26f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-5.8f, 3.2f, 3.25f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("plastic");
	SetShaderMaterial("wood");

	m_basicMeshes->DrawCylinderMesh();

	//Lid middle ring
	scaleXYZ = glm::vec3(0.23f, 0.15f, 0.23f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-5.8f, 3.2f, 3.25f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("plastic");
	SetShaderMaterial("wood");

	m_basicMeshes->DrawCylinderMesh();

	//Lid top ring
	scaleXYZ = glm::vec3(0.21f, 0.1f, 0.21f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-5.8f, 3.3f, 3.25f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("plastic");
	SetShaderMaterial("wood");

	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/

	//Prescription 2 (E): 

	//Orange base
	scaleXYZ = glm::vec3(0.2f, 0.8f, 0.2f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-6.13f, 2.501f, 3.7f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 0.8, 0.5, 0.5);

	m_basicMeshes->DrawCylinderMesh();

	//Paper label
	scaleXYZ = glm::vec3(0.21f, 0.4f, 0.21f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-6.13f, 2.7f, 3.7f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);

	SetShaderTexture("prescription");

	m_basicMeshes->DrawCylinderMesh();

	//Lid bottom ring
	scaleXYZ = glm::vec3(0.26f, 0.03f, 0.26f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-6.13f, 3.2f, 3.7f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("plastic");
	SetShaderMaterial("wood");

	m_basicMeshes->DrawCylinderMesh();

	//Lid middle ring
	scaleXYZ = glm::vec3(0.23f, 0.15f, 0.23f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-6.13f, 3.2f, 3.7f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("plastic");
	SetShaderMaterial("wood");

	m_basicMeshes->DrawCylinderMesh();

	//Lid top ring
	scaleXYZ = glm::vec3(0.21f, 0.1f, 0.21f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-6.13f, 3.3f, 3.7f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("plastic");
	SetShaderMaterial("wood");

	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/

	// Coffee Table

	// Bottom Shelf

	scaleXYZ = glm::vec3(9.0525f, 0.1f, 3.0f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-1.875f, 1.45f, 2.5f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("table");
	SetShaderMaterial("table");

	m_basicMeshes->DrawBoxMesh();

	// Top Shelf

	scaleXYZ = glm::vec3(9.5f, 0.1f, 3.5f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-1.925f, 4.0f, 2.525f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("table");
	SetShaderMaterial("table");

	m_basicMeshes->DrawBoxMesh();

	/*******************************/

	// West Side (camera begins facing east)

	// Top bar

	scaleXYZ = glm::vec3(9.0f, 0.5f, 0.1f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-1.9f, 3.75f, 3.95f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("table");
	SetShaderMaterial("table");

	m_basicMeshes->DrawBoxMesh();

	// Bottom bar

	scaleXYZ = glm::vec3(9.0f, 0.5f, 0.1f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-1.9f, 1.2f, 3.95f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("table");
	SetShaderMaterial("table");

	m_basicMeshes->DrawBoxMesh();

	// Left bar

	scaleXYZ = glm::vec3(0.5f, 4.0f, 0.1f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-6.25f, 2.0f, 4.05f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("table");
	SetShaderMaterial("table");

	m_basicMeshes->DrawBoxMesh();

	// Right bar

	scaleXYZ = glm::vec3(0.5f, 4.0f, 0.1f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(2.5f, 2.0f, 4.05f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("table");
	SetShaderMaterial("table");

	m_basicMeshes->DrawBoxMesh();

	/*******************************/

	// East Side (camera begins facing east)

	// Top bar

	scaleXYZ = glm::vec3(9.0f, 0.5f, 0.1f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-1.9f, 3.75f, 1.05f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("table");
	SetShaderMaterial("table");

	m_basicMeshes->DrawBoxMesh();

	// Bottom bar

	scaleXYZ = glm::vec3(9.0f, 0.5f, 0.1f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-1.9f, 1.2f, 1.05f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("table");
	SetShaderMaterial("table");

	m_basicMeshes->DrawBoxMesh();

	// Left bar

	scaleXYZ = glm::vec3(0.5f, 4.0f, 0.1f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-6.25f, 2.0f, 0.95f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("table");
	SetShaderMaterial("table");

	m_basicMeshes->DrawBoxMesh();

	// Right bar

	scaleXYZ = glm::vec3(0.5f, 4.0f, 0.1f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(2.5f, 2.0f, 0.95f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("table");
	SetShaderMaterial("table");

	m_basicMeshes->DrawBoxMesh();

	/*******************************/

	// North Side (camera begins facing east)

	// Top bar

	scaleXYZ = glm::vec3(0.1f, 0.5f, 3.0f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-6.35f, 3.75f, 2.5f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("table");
	SetShaderMaterial("table");

	m_basicMeshes->DrawBoxMesh();

	// Bottom bar

	scaleXYZ = glm::vec3(0.1f, 0.5f, 3.0f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-6.35f, 1.2f, 2.5f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("table");
	SetShaderMaterial("table");

	m_basicMeshes->DrawBoxMesh();

	// Left bar

	
	scaleXYZ = glm::vec3(0.1f, 4.0f, 0.5f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-6.45f, 2.0f, 1.15f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("table");
	SetShaderMaterial("table");

	m_basicMeshes->DrawBoxMesh();

	// Right bar

	scaleXYZ = glm::vec3(0.1f, 4.0f, 0.5f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-6.45f, 2.0f, 3.85f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("table");
	SetShaderMaterial("table");

	m_basicMeshes->DrawBoxMesh();

	/*******************************/

	// South Side (camera begins facing east)

	// Top bar

	scaleXYZ = glm::vec3(0.1f, 0.5f, 3.0f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(2.6f, 3.75f, 2.5f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("table");
	SetShaderMaterial("table");

	m_basicMeshes->DrawBoxMesh();

	// Bottom bar

	scaleXYZ = glm::vec3(0.1f, 0.5f, 3.0f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(2.6f, 1.2f, 2.5f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("table");
	SetShaderMaterial("table");

	m_basicMeshes->DrawBoxMesh();

	// Left bar


	scaleXYZ = glm::vec3(0.1f, 4.0f, 0.5f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(2.7f, 2.0f, 1.15f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("table");
	SetShaderMaterial("table");

	m_basicMeshes->DrawBoxMesh();

	// Right bar

	scaleXYZ = glm::vec3(0.1f, 4.0f, 0.5f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(2.7f, 2.0f, 3.85f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("table");
	SetShaderMaterial("table");

	m_basicMeshes->DrawBoxMesh();
	
	/****************************************************************/

	// Book

	// Book Bottom

	scaleXYZ = glm::vec3(1.72f, 0.01f, 2.45f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-4.745f, 1.5f, 2.75f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.1, 0.1, 0.1, 1);
	SetShaderMaterial("rug");

	m_basicMeshes->DrawBoxMesh();

	// Book Middle

	scaleXYZ = glm::vec3(1.7f, 0.1f, 2.4f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-4.75f, 1.56f, 2.75f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetShaderMaterial("rug");

	m_basicMeshes->DrawBoxMesh();

	// Book Top

	scaleXYZ = glm::vec3(1.72f, 0.01f, 2.45f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-4.745f, 1.62f, 2.75f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.1, 0.1, 0.1, 1);
	SetShaderMaterial("rug");

	m_basicMeshes->DrawBoxMesh();

	// Book Binding

	scaleXYZ = glm::vec3(0.01f, 0.12f, 2.45f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-5.6f, 1.56f, 2.75f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.1, 0.1, 0.1, 1);
	SetShaderMaterial("rug");

	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/

	// Tablet

	// Tablet Base

	scaleXYZ = glm::vec3(1.2f, 0.01f, 2.0f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-3.0f, 1.505f, 2.75f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.1, 0.1, 0.1, 1);
	SetShaderMaterial("rug");

	m_basicMeshes->DrawBoxMesh();

	// Tablet Screen

	scaleXYZ = glm::vec3(1.2f, 0.0025f, 2.0f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-3.0f, 1.51f, 2.75f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("screen");
	SetShaderMaterial("glass");

	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/

	// TV (Example)

	//scaleXYZ = glm::vec3(5.0f, 5.0f, 0.1f);

	//XrotationDegrees = 0.0f;
	//YrotationDegrees = 0.0f;
	//ZrotationDegrees = 0.0f;

	//positionXYZ = glm::vec3(35.0f, 3.0f, -10.0f);

	//SetTransformations(
	//	scaleXYZ,
	//	XrotationDegrees,
	//	YrotationDegrees,
	//	ZrotationDegrees,
	//	positionXYZ);

	//SetShaderTexture("screen");
	//SetShaderMaterial("glass");

	//m_basicMeshes->DrawBoxMesh();

	/****************************************************************/

	// TV Table

	// Bottom Shelf

	scaleXYZ = glm::vec3(9.0525f, 0.1f, 3.0f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(4.125f, 1.45f, -9.5f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("woodReal");
	SetShaderMaterial("table");

	m_basicMeshes->DrawBoxMesh();

	// Top Shelf

	scaleXYZ = glm::vec3(9.5f, 0.1f, 3.5f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(4.075f, 4.0f, -9.475f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("woodReal");
	SetShaderMaterial("table");

	m_basicMeshes->DrawBoxMesh();

	/*******************************/

	// West Side (camera begins facing east)

	// Top bar

	scaleXYZ = glm::vec3(9.0f, 0.5f, 0.1f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(4.1f, 3.75f, -8.05f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("woodReal");
	SetShaderMaterial("table");

	m_basicMeshes->DrawBoxMesh();

	// Bottom bar

	scaleXYZ = glm::vec3(9.0f, 0.5f, 0.1f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(4.1f, 1.2f, -8.05f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("woodReal");
	SetShaderMaterial("table");

	m_basicMeshes->DrawBoxMesh();

	// Left bar

	scaleXYZ = glm::vec3(0.5f, 4.0f, 0.1f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-0.25f, 2.0f, -7.95f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetTextureUVScale(.1,1);
	SetShaderTexture("woodReal");
	SetShaderMaterial("table");

	m_basicMeshes->DrawBoxMesh();

	// Right bar

	scaleXYZ = glm::vec3(0.5f, 4.0f, 0.1f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(8.5f, 2.0f, -7.95f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("woodReal");
	SetShaderMaterial("table");

	m_basicMeshes->DrawBoxMesh();

	/*******************************/

	// East Side (camera begins facing east)

	// Top bar

	scaleXYZ = glm::vec3(9.0f, 0.5f, 0.1f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(4.1f, 3.75f, -10.95f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetTextureUVScale(1, 1);
	SetShaderTexture("woodReal");
	SetShaderMaterial("table");

	m_basicMeshes->DrawBoxMesh();

	// Bottom bar

	scaleXYZ = glm::vec3(9.0f, 0.5f, 0.1f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(4.1f, 1.2f, -10.95f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("woodReal");
	SetShaderMaterial("table");

	m_basicMeshes->DrawBoxMesh();

	// Left bar

	scaleXYZ = glm::vec3(0.5f, 4.0f, 0.1f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-0.25f, 2.0f, -11.05f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetTextureUVScale(.1, 1);
	SetShaderTexture("woodReal");
	SetShaderMaterial("table");

	m_basicMeshes->DrawBoxMesh();

	// Right bar

	scaleXYZ = glm::vec3(0.5f, 4.0f, 0.1f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(8.5f, 2.0f, -11.05f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("woodReal");
	SetShaderMaterial("table");

	m_basicMeshes->DrawBoxMesh();

	/*******************************/

	// North Side (camera begins facing east)

	// Top bar

	scaleXYZ = glm::vec3(0.1f, 0.5f, 3.0f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-0.35f, 3.75f, -9.5f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetTextureUVScale(1, 1);
	SetShaderTexture("woodReal");
	SetShaderMaterial("table");

	m_basicMeshes->DrawBoxMesh();

	// Bottom bar

	scaleXYZ = glm::vec3(0.1f, 0.5f, 3.0f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-0.35f, 1.2f, -9.5f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("woodReal");
	SetShaderMaterial("table");

	m_basicMeshes->DrawBoxMesh();

	// Left bar

	scaleXYZ = glm::vec3(0.1f, 4.0f, 0.5f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-0.45f, 2.0f, -10.85f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetTextureUVScale(.1, 1);
	SetShaderTexture("woodReal");
	SetShaderMaterial("table");

	m_basicMeshes->DrawBoxMesh();

	// Right bar

	scaleXYZ = glm::vec3(0.1f, 4.0f, 0.5f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-0.45f, 2.0f, -8.15f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("woodReal");
	SetShaderMaterial("table");

	m_basicMeshes->DrawBoxMesh();

	/*******************************/

	// South Side (camera begins facing east)

	// Top bar

	scaleXYZ = glm::vec3(0.1f, 0.5f, 3.0f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(8.6f, 3.75f, -9.5f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetTextureUVScale(1, 1);
	SetShaderTexture("woodReal");
	SetShaderMaterial("table");

	m_basicMeshes->DrawBoxMesh();

	// Bottom bar

	scaleXYZ = glm::vec3(0.1f, 0.5f, 3.0f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(8.6f, 1.2f, -9.5f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("woodReal");
	SetShaderMaterial("table");

	m_basicMeshes->DrawBoxMesh();

	// Left bar


	scaleXYZ = glm::vec3(0.1f, 4.0f, 0.5f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(8.7f, 2.0f, -10.85f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetTextureUVScale(.1, 1);
	SetShaderTexture("woodReal");
	SetShaderMaterial("table");

	m_basicMeshes->DrawBoxMesh();

	// Right bar

	scaleXYZ = glm::vec3(0.1f, 4.0f, 0.5f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(8.7f, 2.0f, -8.15f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("woodReal");
	SetShaderMaterial("table");

	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/

	// Pencil

	// Pencil base

	scaleXYZ = glm::vec3(0.02f, 1.4f, 0.02f);

	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-5.6f, 1.645f, 2.3f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("pencil");
	SetShaderMaterial("table");

	m_basicMeshes->DrawCylinderMesh();

	// Pencil tip

	scaleXYZ = glm::vec3(0.02f, 0.08f, 0.02f);

	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-5.6f, 1.645f, 3.7f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("table");
	SetTextureUVScale(.1,.1);
	SetShaderMaterial("table");

	m_basicMeshes->DrawConeMesh();

	// Pencil eraser

	scaleXYZ = glm::vec3(0.02f, 0.04f, 0.02f);

	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-5.6f, 1.645f, 2.26f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("screen");
	SetShaderMaterial("table");

	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/

}
