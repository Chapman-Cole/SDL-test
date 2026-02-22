#include "MeshObject.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

// The maximum number of vertices that can make up a face in an obj file
#define FACE_VERTEX_MAX 1000

void meshobject_init(Mesh* mesh) {
    mesh->vertexBuffer = NULL;
    mesh->indexBuffer = NULL;
}

void meshobject_destroy(Mesh* mesh) {
    SDL_ReleaseGPUBuffer(get_SDL_gpu_device(), mesh->vertexBuffer);
    SDL_ReleaseGPUBuffer(get_SDL_gpu_device(), mesh->indexBuffer);
}

void meshobject_load_manual(Mesh* mesh, float* vertices, Uint32 vertexSize, Uint32* indices, Uint32 indexSize) {
    mesh->vertexBuffer = GPB_create_buffer(SDL_GPU_BUFFERUSAGE_VERTEX, vertices, vertexSize);
    mesh->indexBuffer = GPB_create_buffer(SDL_GPU_BUFFERUSAGE_INDEX, indices, indexSize);
    mesh->numIndices = indexSize / sizeof(Uint32);
}

// TODO: Consider adding a custom function for calculating how much memory should increase for the vertices
// and triangles arrays so it doesn't grow crazy quick. 
void meshobject_load_objfile(Mesh* mesh, string path) {
    string file;
    string_init(&file);

    string_read_file(&file, &path);

    float* objVertices = NULL;
    Uint32 objVerticesLen = 0;
    Uint32 objVerticesMemsize = 1;
    Uint32* triangles = NULL;
    Uint32 trianglesLen = 0;
    Uint32 trianglesMemsize = 1;

    // The main loop that extracts the data
    for (int i = 0; i < file.len; i++) {
        if (file.str[i] != 'v' && file.str[i] != 'f') {
            // Add the minus 1 so that when 1 is added to i in the loop, it will land
            // on the next vertex (or face)
            int index = string_find_with_offset(&file, &STRING("\n"), i);
            
            // Handle the case when there is no new nextline character
            if (index < 0) {
                index = file.len;
            }

            i = index;
            continue;
        }

        // Handle the extraction of the vertices from the obj file
        if (file.str[i] == 'v') {
            // Stores the indices of the 3 spaces plus the newline character
            Uint32 indices[4];
            Uint32 indexCount = 0;

            Uint32 endLineIndex = string_find_with_offset(&file, &STRING("\n"), i);

            if (endLineIndex < 0) {
                endLineIndex = file.len;
            }

            // Get the indices of the spaces
            for (int j = i + 1; j < endLineIndex && indexCount < 3; j++) {
                if (file.str[j] == ' ') {
                    indices[indexCount] = j;
                    indexCount++;
                }
            }

            indices[3] = endLineIndex;

            // Make sure there is enough room to allocate memory
            if (objVerticesLen + 3 >= objVerticesMemsize) {
                // Memory has to grow fast enough to keep up with the number of vertices being added
                objVerticesMemsize = (objVerticesLen + 3) * 2;
                objVertices = (float*)SDL_realloc(objVertices, objVerticesMemsize * sizeof(float));

                if (objVertices == NULL) {
                    SDL_Log("Failed to allocate memory in meshobject_load_objfile.\n");
                    SDL_Quit();
                    exit(-1);
                }
            }

            // Extract the vertices from the line
            for (int j = 0; j < 3 && objVerticesLen < objVerticesMemsize; j++) {
                string vertexStr;
                string_init(&vertexStr);
                // indices[j] + 1 is because the indices point to the spaces and not the start of the number
                string_substring(&vertexStr, &file, indices[j] + 1, indices[j+1]);
                objVertices[objVerticesLen] = SDL_atof(vertexStr.str);
                objVerticesLen++;
                string_free(&vertexStr);
            }

            i = endLineIndex;
            continue;
        }

        if (file.str[i] == 'f') {
            Uint32 indices[FACE_VERTEX_MAX];
            Uint32 indexCount = 0;

            Uint32 endLineIndex = string_find_with_offset(&file, &STRING("\n"), i);

            if (endLineIndex < 0) {
                endLineIndex = file.len;
            }

            // Get the indices of the spaces. Save room for the \n index by doing FACE_VERTEX_MAX - 1
            for (int j = i + 1; j < endLineIndex && indexCount < FACE_VERTEX_MAX - 1; j++) {
                if (file.str[j] == ' ') {
                    indices[indexCount] = j;
                    indexCount++;
                }
            }

            indices[indexCount] = endLineIndex;
            indexCount++;

            Uint32 faceIndices[FACE_VERTEX_MAX];
            Uint32 faceIndicesCount = 0;

            // Extract the face data to get an array of the face indices.
            for (int j = 0; j < indexCount - 1; j++) {
                string faceIndexStr;
                string_init(&faceIndexStr);
                // indices[j] + 1 is because the indices point to the spaces and not the start of the number
                string_substring(&faceIndexStr, &file, indices[j] + 1, indices[j+1]);

                if (faceIndicesCount < FACE_VERTEX_MAX) {
                    faceIndices[faceIndicesCount] = SDL_atoi(faceIndexStr.str);
                    faceIndicesCount++;
                } else {
                    SDL_Log("Maximum Face Indices of %d reached.", FACE_VERTEX_MAX);
                    SDL_Quit();
                    exit(-1);
                }
                string_free(&faceIndexStr);
            }

            // Make sure there is enough room to allocate memory
            // 3 * (faceIndicesCount - 2) is the number of floats being added, since there are 3 floats per triangle
            if (trianglesLen + 3 * (faceIndicesCount - 2) >= trianglesMemsize) {
                // Memory has to grow fast enough to keep up with number of triangles being added
                trianglesMemsize = (trianglesMemsize + 3 * (faceIndicesCount - 2)) * 2;
                triangles = (Uint32*)SDL_realloc(triangles, trianglesMemsize * sizeof(float));

                if (triangles == NULL) {
                    SDL_Log("Failed to allocate memory in meshobject_load_objfile.\n");
                    SDL_Quit();
                    exit(-1);
                }
            }

            // To get the triangles use pattern like the following. If the face data were 0 1 2 3 4
            // you would get the triangles by doing 0 1 2, 0 2 3, 0 3 4. The number of triangles is equal to the number of vertices on the face
            // which is the number of length of faceIndices minus 2
            for (int j = 0; j < faceIndicesCount - 2; j++) {
                Uint32 subTriangle[3];

                triangles[trianglesLen] = faceIndices[0] - 1;
                triangles[trianglesLen + 1] = faceIndices[j + 1] - 1;
                triangles[trianglesLen + 2] = faceIndices[j + 2] - 1;
                trianglesLen += 3;
            }


            i = endLineIndex;
            continue;
        }
    }

    // Create the vertex and index buffers using the GPB header file
    mesh->vertexBuffer = GPB_create_buffer(SDL_GPU_BUFFERUSAGE_VERTEX, objVertices, objVerticesLen * sizeof(float));
    mesh->indexBuffer = GPB_create_buffer(SDL_GPU_BUFFERUSAGE_INDEX, triangles, trianglesLen * sizeof(Uint32));
    mesh->numIndices = trianglesLen; 

    // Debug print statements are below
    //SDL_Log("Vertices:\n");
    //for (int i = 0; i < objVerticesLen; i += 3) {
    //    SDL_Log("%f %f %f\n", objVertices[i], objVertices[i+1], objVertices[i+2]);
    //}

    //SDL_Log("Triangles:\n");
    //for (int i = 0; i < trianglesLen; i += 3) {
    //    SDL_Log("%d %d %d\n", triangles[i], triangles[i+1], triangles[i+2]);
    //}
    

    // Free up unused data
    SDL_free(objVertices);
    SDL_free(triangles);
    string_free(&file);
}

void meshobject_render(Mesh* mesh, SDL_GPURenderPass* renderPass) {
    // bind the vertex buffer
    SDL_GPUBufferBinding bufferBindings[1];
    bufferBindings[0].buffer = mesh->vertexBuffer;
    bufferBindings[0].offset = 0;

    SDL_GPUBufferBinding indexBinding;
    indexBinding.buffer = mesh->indexBuffer;
    indexBinding.offset = 0;

    SDL_BindGPUVertexBuffers(renderPass, 0, bufferBindings, 1);
    SDL_BindGPUIndexBuffer(renderPass, &indexBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);

    SDL_DrawGPUIndexedPrimitives(renderPass, mesh->numIndices, 1, 0, 0, 0);
}