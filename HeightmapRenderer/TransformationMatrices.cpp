#include "Commons.h"
#include "TransformationMatrices.h"
#include "App.h"

bool TransformationMatrices::modelMatrixChanged = false;
bool TransformationMatrices::projectionMatrixChanged = false;
bool TransformationMatrices::viewMatrixChanged = false;

glm::mat4 TransformationMatrices::model = glm::mat4(1);
glm::mat4 TransformationMatrices::view = glm::mat4(1);
glm::mat4 TransformationMatrices::projection = glm::mat4(1);
glm::mat4 TransformationMatrices::normal = glm::mat4(1);
glm::mat4 TransformationMatrices::modelView = glm::mat4(1);
glm::mat4 TransformationMatrices::modelViewProjection = glm::mat4(1);
glm::mat4 TransformationMatrices::viewProjection = glm::mat4(1);

const glm::mat4 & TransformationMatrices::ModelViewProjection()
{
    viewProjection = projection * view;
    // update frustum planes
    App::Instance()->getCamera().calcPlanes(viewProjection);
    modelViewProjection = viewProjection * model;
    // return new matrix
    return modelViewProjection;
}

const glm::mat4 & TransformationMatrices::ModelView()
{
    modelView = view * model;
    return modelView;
}

const glm::mat4 & TransformationMatrices::Normal()
{
    normal = glm::transpose(glm::inverse(modelView));
    return normal;
}

void TransformationMatrices::Projection(const glm::mat4 &matrix)
{
    projection = matrix;
}

void TransformationMatrices::View(const glm::mat4 &matrix)
{
    view = matrix;
}

void TransformationMatrices::Model(const glm::mat4 &matrix)
{
    model = matrix;
}

TransformationMatrices::TransformationMatrices()
{
}


TransformationMatrices::~TransformationMatrices()
{
}
