#include "model.h"

Node::~Node() {
    for (int i = 0; i < children.size(); i++) {
        delete children[i];
    }
}

Model::Model(QString filename, QOpenGLFunctions_3_3_Core* context) :
    rootNode(0),
    m_fileInfo(filename)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(filename.toStdString(),
                                             aiProcess_Triangulate  |
                                             aiProcess_FlipUVs      |
                                             aiProcess_GenNormals   );

    if (!scene) {
        qDebug() << "Model import error: " << importer.GetErrorString();
        return;
    }

    rootNode = new Node;
    processNode(context, scene->mRootNode, rootNode, scene);
}

Model::~Model() {
    delete rootNode;
    for(int i = 0; i < m_meshes.size(); i++) {
        delete m_meshes[i];
    }
}

void Model::processNode(QOpenGLFunctions_3_3_Core* context, aiNode* source, Node* node, const aiScene* scene) {
    node->name = source->mName.C_Str();

    for(unsigned int i = 0; i < source->mNumMeshes; i++) {
        Mesh* mesh = new Mesh;
        m_meshes.append(mesh);
        node->meshes.append(m_meshes.size() - 1);
        processMesh(scene->mMeshes[source->mMeshes[i]], mesh, scene);
        mesh->setup(context);
    }

    for(unsigned int i = 0; i < source->mNumChildren; i++) {
        Node* child = new Node;
        node->children.append(child);
        processNode(context, source->mChildren[i], child, scene);
    }
}

void Model::processMesh(aiMesh* source, Mesh* mesh, const aiScene* scene) {
    for(unsigned int i = 0; i < source->mNumVertices; i++) {
        Vertex vertex;

        aiVector3D pos = source->mVertices[i];
        aiVector3D normal = source->mNormals[i];
        vertex.position = QVector3D(pos.x, pos.y, pos.z);
        vertex.normal = QVector3D(normal.x, normal.y, normal.z);

        if (source->mTextureCoords[0]) {
            aiVector3D tex = source->mTextureCoords[0][i];
            vertex.texCoords = QVector2D(tex.x, tex.y);
        }

        mesh->vertices.append(vertex);
    }

    for(unsigned int i = 0; i < source->mNumFaces; i++) {
        aiFace face = source->mFaces[i];
        for(unsigned int j = 0; j < face.mNumIndices; j++)
            mesh->indices.append(face.mIndices[j]);
    }

    aiMaterial* material = scene->mMaterials[source->mMaterialIndex];
    loadTextures(mesh, material, aiTextureType_DIFFUSE);
}

void Model::loadTextures(Mesh* mesh, aiMaterial* material, aiTextureType type) {
    QDir dir = m_fileInfo.dir();

    for(unsigned int i = 0; i < material->GetTextureCount(type); i++) {
        aiString filename;

        material->GetTexture(type, i, &filename);
        QImage tex(dir.absoluteFilePath(filename.C_Str()));

        if (tex.isNull()) {
            qDebug() << "Texture not found : " << filename.C_Str();
            continue;
        }

        QOpenGLTexture* texture = new QOpenGLTexture(tex, QOpenGLTexture::DontGenerateMipMaps);
        mesh->textures.append(texture);
    }
}

void Model::draw(QOpenGLFunctions_3_3_Core* context, QOpenGLShaderProgram* program) {
    drawNode(rootNode, QMatrix4x4(), context, program);
}

void Model::drawNode(Node* node, QMatrix4x4 transform, QOpenGLFunctions_3_3_Core* context, QOpenGLShaderProgram* program) {
    program->setUniformValue("node", node->transformation * transform);
    for(int i = 0; i < node->meshes.size(); i++) {
        m_meshes[node->meshes[i]]->draw(context);
    }
    for(int i = 0; i < node->children.size(); i++) {
        drawNode(node->children[i], node->transformation * transform, context, program);
    }
}
