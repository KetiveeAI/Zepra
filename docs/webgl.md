1 Introduction
WebGL™ is an immediate mode 3D rendering API designed for the web. This is Version 2 of the WebGL specification. It is derived from OpenGL® ES 3.0, and provides similar rendering functionality, but in an HTML context.

WebGL 2.0 is not entirely backwards compatible with WebGL 1.0. Existing error-free content written against the core WebGL 1.0 specification without extensions will often run in WebGL 2.0 without modification, but this is not always the case. All exceptions to backwards compatibility are recorded in the Backwards Incompatibility section. To access the new behavior provided in this specification, the content explicitly requests a new context (details below).

1.1 Conventions
Many functions described in this document contain links to OpenGL ES man pages. While every effort is made to make these pages match the OpenGL ES 3.0 specification [GLES30], they may contain errors. In the case of a contradiction, the OpenGL ES 3.0 specification is the final authority.

The remaining sections of this document are intended to be read in conjunction with the OpenGL ES 3.0 specification (3.0.6 at the time of this writing, available from the Khronos OpenGL ES API Registry). Unless otherwise specified, the behavior of each method is defined by the OpenGL ES 3.0 specification. This specification may diverge from OpenGL ES 3.0 in order to ensure interoperability or security, often defining areas that OpenGL ES 3.0 leaves implementation-defined. These differences are summarized in the Differences Between WebGL and OpenGL ES 3.0 section.

2 Context Creation and Drawing Buffer Presentation
Before using the WebGL API, the author must obtain a WebGLRenderingContext object for a given HTMLCanvasElement [CANVAS] or OffscreenCanvas [OFFSCREENCANVAS] as described below. This object is used to manage OpenGL state and render to the drawing buffer, which must be created at the time of context creation.

2.1 Context Creation
Each WebGLRenderingContext and WebGL2RenderingContext has an associated canvas, set upon creation, which is a canvas [CANVAS] or offscreen canvas [OFFSCREENCANVAS].

Each WebGLRenderingContext and WebGL2RenderingContext has context creation parameters, set upon creation, in a WebGLContextAttributes object.

Each WebGLRenderingContext and WebGL2RenderingContext has actual context parameters, set each time the drawing buffer is created, in a WebGLContextAttributes object.

Each WebGLRenderingContext and WebGL2RenderingContext has a webgl context lost flag, which is initially unset.

When the getContext() method of a canvas element is to return a new object for the contextId webgl2 [CANVASCONTEXTS], the user agent must perform the following steps:

 Create a new WebGL2RenderingContext object, context.
 Let context's canvas be the canvas or offscreen canvas the getContext() method is associated with.
 Create a new WebGLContextAttributes object, contextAttributes.
 If getContext() was invoked with a second argument, options, set the attributes of contextAttributes from those specified in options.
 Create a drawing buffer using the settings specified in contextAttributes, and associate the drawing buffer with context.
 If drawing buffer creation failed, perform the following steps:
 Fire a WebGL context creation error at canvas.
 Return null and terminate these steps.
 Create a new WebGLContextAttributes object, actualAttributes.
 Set the attributes of actualAttributes based on the properties of the newly created drawing buffer.
 Set context's context creation parameters to contextAttributes.
 Set context's actual context parameters to actualAttributes.
 Return context.
2.2 The Drawing Buffer
Different from WebGL 1.0, the depth, stencil, and antialias attributes in WebGL 2.0 must be obeyed by the WebGL implementation.

3 DOM Interfaces
This section describes the interfaces and functionality added to the DOM to support runtime access to the functionality described above.

3.1 Types
The following types are introduced in WebGL 2.0.

typedef long long GLint64;
typedef unsigned long long GLuint64;
3.2 WebGLQuery
The WebGLQuery interface represents an OpenGL Query Object. The underlying object is created as if by calling glGenQueries (OpenGL ES 3.0.6 §2.14, man page) , made active as if by calling glBeginQuery (OpenGL ES 3.0.6 §2.14, man page) , concluded as if by calling glEndQuery (OpenGL ES 3.0.6 §2.14, man page) and destroyed as if by calling glDeleteQueries (OpenGL ES 3.0.6 §2.14, man page) .

[Exposed=(Window,Worker)]
interface WebGLQuery : WebGLObject {
};
3.3 WebGLSampler
The WebGLSampler interface represents an OpenGL Sampler Object. The underlying object is created as if by calling glGenSamplers (OpenGL ES 3.0.6 §3.8.2, man page) , bound as if by calling glBindSampler (OpenGL ES 3.0.6 §3.8.2, man page) and destroyed as if by calling glDeleteSamplers (OpenGL ES 3.0.6 §3.8.2, man page) .

[Exposed=(Window,Worker)]
interface WebGLSampler : WebGLObject {
};
3.4 WebGLSync
The WebGLSync interface represents an OpenGL Sync Object. The underlying object is created as if by calling glFenceSync (OpenGL ES 3.0.6 §5.2, man page) , blocked on as if by calling glClientWaitSync (OpenGL ES 3.0.6 §5.2.1, man page) , waited on internal to GL as if by calling glWaitSync (OpenGL ES 3.0.6 §5.2.1, man page) , queried as if by calling glGetSynciv (OpenGL ES 3.0.6 §6.1.8, man page) , and destroyed as if by calling glDeleteSync (OpenGL ES 3.0.6 §5.2, man page) .


[Exposed=(Window,Worker)]
interface WebGLSync : WebGLObject {
};
3.5 WebGLTransformFeedback
The WebGLTransformFeedback interface represents an OpenGL Transform Feedback Object. The underlying object is created as if by calling glGenTransformFeedbacks (OpenGL ES 3.0.6 §2.15.1, man page) , bound as if by calling glBindTransformFeedback (OpenGL ES 3.0.6 §2.15.1, man page) and destroyed as if by calling glDeleteTransformFeedbacks (OpenGL ES 3.0.6 §2.15.1, man page) .

[Exposed=(Window,Worker)]
interface WebGLTransformFeedback : WebGLObject {
};
3.6 WebGLVertexArrayObject
The WebGLVertexArrayObject interface represents an OpenGL Vertex Array Object. The underlying object is created as if by calling glGenVertexArrays (OpenGL ES 3.0.6 §2.10, man page) , bound as if by calling glBindVertexArray (OpenGL ES 3.0.6 §2.10, man page) and destroyed as if by calling glDeleteVertexArrays (OpenGL ES 3.0.6 §2.10, man page) .

[Exposed=(Window,Worker)]
interface WebGLVertexArrayObject : WebGLObject {
};
3.7 The WebGL context
The WebGL2RenderingContext represents the API allowing OpenGL ES 3.0 style rendering into the canvas element.

typedef ([AllowShared] Uint32Array or sequence<GLuint>) Uint32List;

interface mixin WebGL2RenderingContextBase
{
  const GLenum READ_BUFFER                                   = 0x0C02;
  const GLenum UNPACK_ROW_LENGTH                             = 0x0CF2;
  const GLenum UNPACK_SKIP_ROWS                              = 0x0CF3;
  const GLenum UNPACK_SKIP_PIXELS                            = 0x0CF4;
  const GLenum PACK_ROW_LENGTH                               = 0x0D02;
  const GLenum PACK_SKIP_ROWS                                = 0x0D03;
  const GLenum PACK_SKIP_PIXELS                              = 0x0D04;
  const GLenum COLOR                                         = 0x1800;
  const GLenum DEPTH                                         = 0x1801;
  const GLenum STENCIL                                       = 0x1802;
  const GLenum RED                                           = 0x1903;
  const GLenum RGB8                                          = 0x8051;
  const GLenum RGB10_A2                                      = 0x8059;
  const GLenum TEXTURE_BINDING_3D                            = 0x806A;
  const GLenum UNPACK_SKIP_IMAGES                            = 0x806D;
  const GLenum UNPACK_IMAGE_HEIGHT                           = 0x806E;
  const GLenum TEXTURE_3D                                    = 0x806F;
  const GLenum TEXTURE_WRAP_R                                = 0x8072;
  const GLenum MAX_3D_TEXTURE_SIZE                           = 0x8073;
  const GLenum UNSIGNED_INT_2_10_10_10_REV                   = 0x8368;
  const GLenum MAX_ELEMENTS_VERTICES                         = 0x80E8;
  const GLenum MAX_ELEMENTS_INDICES                          = 0x80E9;
  const GLenum TEXTURE_MIN_LOD                               = 0x813A;
  const GLenum TEXTURE_MAX_LOD                               = 0x813B;
  const GLenum TEXTURE_BASE_LEVEL                            = 0x813C;
  const GLenum TEXTURE_MAX_LEVEL                             = 0x813D;
  const GLenum MIN                                           = 0x8007;
  const GLenum MAX                                           = 0x8008;
  const GLenum DEPTH_COMPONENT24                             = 0x81A6;
  const GLenum MAX_TEXTURE_LOD_BIAS                          = 0x84FD;
  const GLenum TEXTURE_COMPARE_MODE                          = 0x884C;
  const GLenum TEXTURE_COMPARE_FUNC                          = 0x884D;
  const GLenum CURRENT_QUERY                                 = 0x8865;
  const GLenum QUERY_RESULT                                  = 0x8866;
  const GLenum QUERY_RESULT_AVAILABLE                        = 0x8867;
  const GLenum STREAM_READ                                   = 0x88E1;
  const GLenum STREAM_COPY                                   = 0x88E2;
  const GLenum STATIC_READ                                   = 0x88E5;
  const GLenum STATIC_COPY                                   = 0x88E6;
  const GLenum DYNAMIC_READ                                  = 0x88E9;
  const GLenum DYNAMIC_COPY                                  = 0x88EA;
  const GLenum MAX_DRAW_BUFFERS                              = 0x8824;
  const GLenum DRAW_BUFFER0                                  = 0x8825;
  const GLenum DRAW_BUFFER1                                  = 0x8826;
  const GLenum DRAW_BUFFER2                                  = 0x8827;
  const GLenum DRAW_BUFFER3                                  = 0x8828;
  const GLenum DRAW_BUFFER4                                  = 0x8829;
  const GLenum DRAW_BUFFER5                                  = 0x882A;
  const GLenum DRAW_BUFFER6                                  = 0x882B;
  const GLenum DRAW_BUFFER7                                  = 0x882C;
  const GLenum DRAW_BUFFER8                                  = 0x882D;
  const GLenum DRAW_BUFFER9                                  = 0x882E;
  const GLenum DRAW_BUFFER10                                 = 0x882F;
  const GLenum DRAW_BUFFER11                                 = 0x8830;
  const GLenum DRAW_BUFFER12                                 = 0x8831;
  const GLenum DRAW_BUFFER13                                 = 0x8832;
  const GLenum DRAW_BUFFER14                                 = 0x8833;
  const GLenum DRAW_BUFFER15                                 = 0x8834;
  const GLenum MAX_FRAGMENT_UNIFORM_COMPONENTS               = 0x8B49;
  const GLenum MAX_VERTEX_UNIFORM_COMPONENTS                 = 0x8B4A;
  const GLenum SAMPLER_3D                                    = 0x8B5F;
  const GLenum SAMPLER_2D_SHADOW                             = 0x8B62;
  const GLenum FRAGMENT_SHADER_DERIVATIVE_HINT               = 0x8B8B;
  const GLenum PIXEL_PACK_BUFFER                             = 0x88EB;
  const GLenum PIXEL_UNPACK_BUFFER                           = 0x88EC;
  const GLenum PIXEL_PACK_BUFFER_BINDING                     = 0x88ED;
  const GLenum PIXEL_UNPACK_BUFFER_BINDING                   = 0x88EF;
  const GLenum FLOAT_MAT2x3                                  = 0x8B65;
  const GLenum FLOAT_MAT2x4                                  = 0x8B66;
  const GLenum FLOAT_MAT3x2                                  = 0x8B67;
  const GLenum FLOAT_MAT3x4                                  = 0x8B68;
  const GLenum FLOAT_MAT4x2                                  = 0x8B69;
  const GLenum FLOAT_MAT4x3                                  = 0x8B6A;
  const GLenum SRGB                                          = 0x8C40;
  const GLenum SRGB8                                         = 0x8C41;
  const GLenum SRGB8_ALPHA8                                  = 0x8C43;
  const GLenum COMPARE_REF_TO_TEXTURE                        = 0x884E;
  const GLenum RGBA32F                                       = 0x8814;
  const GLenum RGB32F                                        = 0x8815;
  const GLenum RGBA16F                                       = 0x881A;
  const GLenum RGB16F                                        = 0x881B;
  const GLenum VERTEX_ATTRIB_ARRAY_INTEGER                   = 0x88FD;
  const GLenum MAX_ARRAY_TEXTURE_LAYERS                      = 0x88FF;
  const GLenum MIN_PROGRAM_TEXEL_OFFSET                      = 0x8904;
  const GLenum MAX_PROGRAM_TEXEL_OFFSET                      = 0x8905;
  const GLenum MAX_VARYING_COMPONENTS                        = 0x8B4B;
  const GLenum TEXTURE_2D_ARRAY                              = 0x8C1A;
  const GLenum TEXTURE_BINDING_2D_ARRAY                      = 0x8C1D;
  const GLenum R11F_G11F_B10F                                = 0x8C3A;
  const GLenum UNSIGNED_INT_10F_11F_11F_REV                  = 0x8C3B;
  const GLenum RGB9_E5                                       = 0x8C3D;
  const GLenum UNSIGNED_INT_5_9_9_9_REV                      = 0x8C3E;
  const GLenum TRANSFORM_FEEDBACK_BUFFER_MODE                = 0x8C7F;
  const GLenum MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS    = 0x8C80;
  const GLenum TRANSFORM_FEEDBACK_VARYINGS                   = 0x8C83;
  const GLenum TRANSFORM_FEEDBACK_BUFFER_START               = 0x8C84;
  const GLenum TRANSFORM_FEEDBACK_BUFFER_SIZE                = 0x8C85;
  const GLenum TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN         = 0x8C88;
  const GLenum RASTERIZER_DISCARD                            = 0x8C89;
  const GLenum MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS = 0x8C8A;
  const GLenum MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS       = 0x8C8B;
  const GLenum INTERLEAVED_ATTRIBS                           = 0x8C8C;
  const GLenum SEPARATE_ATTRIBS                              = 0x8C8D;
  const GLenum TRANSFORM_FEEDBACK_BUFFER                     = 0x8C8E;
  const GLenum TRANSFORM_FEEDBACK_BUFFER_BINDING             = 0x8C8F;
  const GLenum RGBA32UI                                      = 0x8D70;
  const GLenum RGB32UI                                       = 0x8D71;
  const GLenum RGBA16UI                                      = 0x8D76;
  const GLenum RGB16UI                                       = 0x8D77;
  const GLenum RGBA8UI                                       = 0x8D7C;
  const GLenum RGB8UI                                        = 0x8D7D;
  const GLenum RGBA32I                                       = 0x8D82;
  const GLenum RGB32I                                        = 0x8D83;
  const GLenum RGBA16I                                       = 0x8D88;
  const GLenum RGB16I                                        = 0x8D89;
  const GLenum RGBA8I                                        = 0x8D8E;
  const GLenum RGB8I                                         = 0x8D8F;
  const GLenum RED_INTEGER                                   = 0x8D94;
  const GLenum RGB_INTEGER                                   = 0x8D98;
  const GLenum RGBA_INTEGER                                  = 0x8D99;
  const GLenum SAMPLER_2D_ARRAY                              = 0x8DC1;
  const GLenum SAMPLER_2D_ARRAY_SHADOW                       = 0x8DC4;
  const GLenum SAMPLER_CUBE_SHADOW                           = 0x8DC5;
  const GLenum UNSIGNED_INT_VEC2                             = 0x8DC6;
  const GLenum UNSIGNED_INT_VEC3                             = 0x8DC7;
  const GLenum UNSIGNED_INT_VEC4                             = 0x8DC8;
  const GLenum INT_SAMPLER_2D                                = 0x8DCA;
  const GLenum INT_SAMPLER_3D                                = 0x8DCB;
  const GLenum INT_SAMPLER_CUBE                              = 0x8DCC;
  const GLenum INT_SAMPLER_2D_ARRAY                          = 0x8DCF;
  const GLenum UNSIGNED_INT_SAMPLER_2D                       = 0x8DD2;
  const GLenum UNSIGNED_INT_SAMPLER_3D                       = 0x8DD3;
  const GLenum UNSIGNED_INT_SAMPLER_CUBE                     = 0x8DD4;
  const GLenum UNSIGNED_INT_SAMPLER_2D_ARRAY                 = 0x8DD7;
  const GLenum DEPTH_COMPONENT32F                            = 0x8CAC;
  const GLenum DEPTH32F_STENCIL8                             = 0x8CAD;
  const GLenum FLOAT_32_UNSIGNED_INT_24_8_REV                = 0x8DAD;
  const GLenum FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING         = 0x8210;
  const GLenum FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE         = 0x8211;
  const GLenum FRAMEBUFFER_ATTACHMENT_RED_SIZE               = 0x8212;
  const GLenum FRAMEBUFFER_ATTACHMENT_GREEN_SIZE             = 0x8213;
  const GLenum FRAMEBUFFER_ATTACHMENT_BLUE_SIZE              = 0x8214;
  const GLenum FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE             = 0x8215;
  const GLenum FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE             = 0x8216;
  const GLenum FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE           = 0x8217;
  const GLenum FRAMEBUFFER_DEFAULT                           = 0x8218;
  const GLenum UNSIGNED_INT_24_8                             = 0x84FA;
  const GLenum DEPTH24_STENCIL8                              = 0x88F0;
  const GLenum UNSIGNED_NORMALIZED                           = 0x8C17;
  const GLenum DRAW_FRAMEBUFFER_BINDING                      = 0x8CA6; /* Same as FRAMEBUFFER_BINDING */
  const GLenum READ_FRAMEBUFFER                              = 0x8CA8;
  const GLenum DRAW_FRAMEBUFFER                              = 0x8CA9;
  const GLenum READ_FRAMEBUFFER_BINDING                      = 0x8CAA;
  const GLenum RENDERBUFFER_SAMPLES                          = 0x8CAB;
  const GLenum FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER          = 0x8CD4;
  const GLenum MAX_COLOR_ATTACHMENTS                         = 0x8CDF;
  const GLenum COLOR_ATTACHMENT1                             = 0x8CE1;
  const GLenum COLOR_ATTACHMENT2                             = 0x8CE2;
  const GLenum COLOR_ATTACHMENT3                             = 0x8CE3;
  const GLenum COLOR_ATTACHMENT4                             = 0x8CE4;
  const GLenum COLOR_ATTACHMENT5                             = 0x8CE5;
  const GLenum COLOR_ATTACHMENT6                             = 0x8CE6;
  const GLenum COLOR_ATTACHMENT7                             = 0x8CE7;
  const GLenum COLOR_ATTACHMENT8                             = 0x8CE8;
  const GLenum COLOR_ATTACHMENT9                             = 0x8CE9;
  const GLenum COLOR_ATTACHMENT10                            = 0x8CEA;
  const GLenum COLOR_ATTACHMENT11                            = 0x8CEB;
  const GLenum COLOR_ATTACHMENT12                            = 0x8CEC;
  const GLenum COLOR_ATTACHMENT13                            = 0x8CED;
  const GLenum COLOR_ATTACHMENT14                            = 0x8CEE;
  const GLenum COLOR_ATTACHMENT15                            = 0x8CEF;
  const GLenum FRAMEBUFFER_INCOMPLETE_MULTISAMPLE            = 0x8D56;
  const GLenum MAX_SAMPLES                                   = 0x8D57;
  const GLenum HALF_FLOAT                                    = 0x140B;
  const GLenum RG                                            = 0x8227;
  const GLenum RG_INTEGER                                    = 0x8228;
  const GLenum R8                                            = 0x8229;
  const GLenum RG8                                           = 0x822B;
  const GLenum R16F                                          = 0x822D;
  const GLenum R32F                                          = 0x822E;
  const GLenum RG16F                                         = 0x822F;
  const GLenum RG32F                                         = 0x8230;
  const GLenum R8I                                           = 0x8231;
  const GLenum R8UI                                          = 0x8232;
  const GLenum R16I                                          = 0x8233;
  const GLenum R16UI                                         = 0x8234;
  const GLenum R32I                                          = 0x8235;
  const GLenum R32UI                                         = 0x8236;
  const GLenum RG8I                                          = 0x8237;
  const GLenum RG8UI                                         = 0x8238;
  const GLenum RG16I                                         = 0x8239;
  const GLenum RG16UI                                        = 0x823A;
  const GLenum RG32I                                         = 0x823B;
  const GLenum RG32UI                                        = 0x823C;
  const GLenum VERTEX_ARRAY_BINDING                          = 0x85B5;
  const GLenum R8_SNORM                                      = 0x8F94;
  const GLenum RG8_SNORM                                     = 0x8F95;
  const GLenum RGB8_SNORM                                    = 0x8F96;
  const GLenum RGBA8_SNORM                                   = 0x8F97;
  const GLenum SIGNED_NORMALIZED                             = 0x8F9C;
  const GLenum COPY_READ_BUFFER                              = 0x8F36;
  const GLenum COPY_WRITE_BUFFER                             = 0x8F37;
  const GLenum COPY_READ_BUFFER_BINDING                      = 0x8F36; /* Same as COPY_READ_BUFFER */
  const GLenum COPY_WRITE_BUFFER_BINDING                     = 0x8F37; /* Same as COPY_WRITE_BUFFER */
  const GLenum UNIFORM_BUFFER                                = 0x8A11;
  const GLenum UNIFORM_BUFFER_BINDING                        = 0x8A28;
  const GLenum UNIFORM_BUFFER_START                          = 0x8A29;
  const GLenum UNIFORM_BUFFER_SIZE                           = 0x8A2A;
  const GLenum MAX_VERTEX_UNIFORM_BLOCKS                     = 0x8A2B;
  const GLenum MAX_FRAGMENT_UNIFORM_BLOCKS                   = 0x8A2D;
  const GLenum MAX_COMBINED_UNIFORM_BLOCKS                   = 0x8A2E;
  const GLenum MAX_UNIFORM_BUFFER_BINDINGS                   = 0x8A2F;
  const GLenum MAX_UNIFORM_BLOCK_SIZE                        = 0x8A30;
  const GLenum MAX_COMBINED_VERTEX_UNIFORM_COMPONENTS        = 0x8A31;
  const GLenum MAX_COMBINED_FRAGMENT_UNIFORM_COMPONENTS      = 0x8A33;
  const GLenum UNIFORM_BUFFER_OFFSET_ALIGNMENT               = 0x8A34;
  const GLenum ACTIVE_UNIFORM_BLOCKS                         = 0x8A36;
  const GLenum UNIFORM_TYPE                                  = 0x8A37;
  const GLenum UNIFORM_SIZE                                  = 0x8A38;
  const GLenum UNIFORM_BLOCK_INDEX                           = 0x8A3A;
  const GLenum UNIFORM_OFFSET                                = 0x8A3B;
  const GLenum UNIFORM_ARRAY_STRIDE                          = 0x8A3C;
  const GLenum UNIFORM_MATRIX_STRIDE                         = 0x8A3D;
  const GLenum UNIFORM_IS_ROW_MAJOR                          = 0x8A3E;
  const GLenum UNIFORM_BLOCK_BINDING                         = 0x8A3F;
  const GLenum UNIFORM_BLOCK_DATA_SIZE                       = 0x8A40;
  const GLenum UNIFORM_BLOCK_ACTIVE_UNIFORMS                 = 0x8A42;
  const GLenum UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES          = 0x8A43;
  const GLenum UNIFORM_BLOCK_REFERENCED_BY_VERTEX_SHADER     = 0x8A44;
  const GLenum UNIFORM_BLOCK_REFERENCED_BY_FRAGMENT_SHADER   = 0x8A46;
  const GLenum INVALID_INDEX                                 = 0xFFFFFFFF;
  const GLenum MAX_VERTEX_OUTPUT_COMPONENTS                  = 0x9122;
  const GLenum MAX_FRAGMENT_INPUT_COMPONENTS                 = 0x9125;
  const GLenum MAX_SERVER_WAIT_TIMEOUT                       = 0x9111;
  const GLenum OBJECT_TYPE                                   = 0x9112;
  const GLenum SYNC_CONDITION                                = 0x9113;
  const GLenum SYNC_STATUS                                   = 0x9114;
  const GLenum SYNC_FLAGS                                    = 0x9115;
  const GLenum SYNC_FENCE                                    = 0x9116;
  const GLenum SYNC_GPU_COMMANDS_COMPLETE                    = 0x9117;
  const GLenum UNSIGNALED                                    = 0x9118;
  const GLenum SIGNALED                                      = 0x9119;
  const GLenum ALREADY_SIGNALED                              = 0x911A;
  const GLenum TIMEOUT_EXPIRED                               = 0x911B;
  const GLenum CONDITION_SATISFIED                           = 0x911C;
  const GLenum WAIT_FAILED                                   = 0x911D;
  const GLenum SYNC_FLUSH_COMMANDS_BIT                       = 0x00000001;
  const GLenum VERTEX_ATTRIB_ARRAY_DIVISOR                   = 0x88FE;
  const GLenum ANY_SAMPLES_PASSED                            = 0x8C2F;
  const GLenum ANY_SAMPLES_PASSED_CONSERVATIVE               = 0x8D6A;
  const GLenum SAMPLER_BINDING                               = 0x8919;
  const GLenum RGB10_A2UI                                    = 0x906F;
  const GLenum INT_2_10_10_10_REV                            = 0x8D9F;
  const GLenum TRANSFORM_FEEDBACK                            = 0x8E22;
  const GLenum TRANSFORM_FEEDBACK_PAUSED                     = 0x8E23;
  const GLenum TRANSFORM_FEEDBACK_ACTIVE                     = 0x8E24;
  const GLenum TRANSFORM_FEEDBACK_BINDING                    = 0x8E25;
  const GLenum TEXTURE_IMMUTABLE_FORMAT                      = 0x912F;
  const GLenum MAX_ELEMENT_INDEX                             = 0x8D6B;
  const GLenum TEXTURE_IMMUTABLE_LEVELS                      = 0x82DF;

  const GLint64 TIMEOUT_IGNORED                              = -1;

  /* WebGL-specific enums */
  const GLenum MAX_CLIENT_WAIT_TIMEOUT_WEBGL                 = 0x9247;

  /* Buffer objects */
  undefined copyBufferSubData(GLenum readTarget, GLenum writeTarget, GLintptr readOffset,
                              GLintptr writeOffset, GLsizeiptr size);
  // MapBufferRange, in particular its read-only and write-only modes,
  // can not be exposed safely to JavaScript. GetBufferSubData
  // replaces it for the purpose of fetching data back from the GPU.
  undefined getBufferSubData(GLenum target, GLintptr srcByteOffset, [AllowShared] ArrayBufferView dstBuffer,
                             optional unsigned long long dstOffset = 0, optional GLuint length = 0);

  /* Framebuffer objects */
  undefined blitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0,
                            GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
  undefined framebufferTextureLayer(GLenum target, GLenum attachment, WebGLTexture? texture, GLint level,
                                    GLint layer);
  undefined invalidateFramebuffer(GLenum target, sequence<GLenum> attachments);
  undefined invalidateSubFramebuffer(GLenum target, sequence<GLenum> attachments,
                                     GLint x, GLint y, GLsizei width, GLsizei height);
  undefined readBuffer(GLenum src);

  /* Renderbuffer objects */
  any getInternalformatParameter(GLenum target, GLenum internalformat, GLenum pname);
  undefined renderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalformat,
                                           GLsizei width, GLsizei height);

  /* Texture objects */
  undefined texStorage2D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width,
                         GLsizei height);
  undefined texStorage3D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width,
                         GLsizei height, GLsizei depth);

  undefined texImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height,
                       GLsizei depth, GLint border, GLenum format, GLenum type, GLintptr pboOffset);
  undefined texImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height,
                       GLsizei depth, GLint border, GLenum format, GLenum type,
                       TexImageSource source); // May throw DOMException
  undefined texImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height,
                       GLsizei depth, GLint border, GLenum format, GLenum type, [AllowShared] ArrayBufferView? srcData);
  undefined texImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height,
                       GLsizei depth, GLint border, GLenum format, GLenum type, [AllowShared] ArrayBufferView srcData,
                       unsigned long long srcOffset);

  undefined texSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
                          GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type,
                          GLintptr pboOffset);
  undefined texSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
                          GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type,
                          TexImageSource source); // May throw DOMException
  undefined texSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
                          GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type,
                          [AllowShared] ArrayBufferView? srcData, optional unsigned long long srcOffset = 0);

  undefined copyTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
                              GLint x, GLint y, GLsizei width, GLsizei height);

  undefined compressedTexImage3D(GLenum target, GLint level, GLenum internalformat, GLsizei width,
                                 GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, GLintptr offset);
  undefined compressedTexImage3D(GLenum target, GLint level, GLenum internalformat, GLsizei width,
                                 GLsizei height, GLsizei depth, GLint border, [AllowShared] ArrayBufferView srcData,
                                 optional unsigned long long srcOffset = 0, optional GLuint srcLengthOverride = 0);

  undefined compressedTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                                    GLint zoffset, GLsizei width, GLsizei height, GLsizei depth,
                                    GLenum format, GLsizei imageSize, GLintptr offset);
  undefined compressedTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                                    GLint zoffset, GLsizei width, GLsizei height, GLsizei depth,
                                    GLenum format, [AllowShared] ArrayBufferView srcData,
                                    optional unsigned long long srcOffset = 0,
                                    optional GLuint srcLengthOverride = 0);

  /* Programs and shaders */
  [WebGLHandlesContextLoss] GLint getFragDataLocation(WebGLProgram program, DOMString name);

  /* Uniforms */
  undefined uniform1ui(WebGLUniformLocation? location, GLuint v0);
  undefined uniform2ui(WebGLUniformLocation? location, GLuint v0, GLuint v1);
  undefined uniform3ui(WebGLUniformLocation? location, GLuint v0, GLuint v1, GLuint v2);
  undefined uniform4ui(WebGLUniformLocation? location, GLuint v0, GLuint v1, GLuint v2, GLuint v3);

  undefined uniform1uiv(WebGLUniformLocation? location, Uint32List data, optional unsigned long long srcOffset = 0,
                        optional GLuint srcLength = 0);
  undefined uniform2uiv(WebGLUniformLocation? location, Uint32List data, optional unsigned long long srcOffset = 0,
                        optional GLuint srcLength = 0);
  undefined uniform3uiv(WebGLUniformLocation? location, Uint32List data, optional unsigned long long srcOffset = 0,
                        optional GLuint srcLength = 0);
  undefined uniform4uiv(WebGLUniformLocation? location, Uint32List data, optional unsigned long long srcOffset = 0,
                        optional GLuint srcLength = 0);
  undefined uniformMatrix3x2fv(WebGLUniformLocation? location, GLboolean transpose, Float32List data,
                               optional unsigned long long srcOffset = 0, optional GLuint srcLength = 0);
  undefined uniformMatrix4x2fv(WebGLUniformLocation? location, GLboolean transpose, Float32List data,
                               optional unsigned long long srcOffset = 0, optional GLuint srcLength = 0);

  undefined uniformMatrix2x3fv(WebGLUniformLocation? location, GLboolean transpose, Float32List data,
                               optional unsigned long long srcOffset = 0, optional GLuint srcLength = 0);
  undefined uniformMatrix4x3fv(WebGLUniformLocation? location, GLboolean transpose, Float32List data,
                               optional unsigned long long srcOffset = 0, optional GLuint srcLength = 0);

  undefined uniformMatrix2x4fv(WebGLUniformLocation? location, GLboolean transpose, Float32List data,
                               optional unsigned long long srcOffset = 0, optional GLuint srcLength = 0);
  undefined uniformMatrix3x4fv(WebGLUniformLocation? location, GLboolean transpose, Float32List data,
                               optional unsigned long long srcOffset = 0, optional GLuint srcLength = 0);

  /* Vertex attribs */
  undefined vertexAttribI4i(GLuint index, GLint x, GLint y, GLint z, GLint w);
  undefined vertexAttribI4iv(GLuint index, Int32List values);
  undefined vertexAttribI4ui(GLuint index, GLuint x, GLuint y, GLuint z, GLuint w);
  undefined vertexAttribI4uiv(GLuint index, Uint32List values);
  undefined vertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride, GLintptr offset);

  /* Writing to the drawing buffer */
  undefined vertexAttribDivisor(GLuint index, GLuint divisor);
  undefined drawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei instanceCount);
  undefined drawElementsInstanced(GLenum mode, GLsizei count, GLenum type, GLintptr offset, GLsizei instanceCount);
  undefined drawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, GLintptr offset);

  /* Multiple Render Targets */
  undefined drawBuffers(sequence<GLenum> buffers);

  undefined clearBufferfv(GLenum buffer, GLint drawbuffer, Float32List values,
                          optional unsigned long long srcOffset = 0);
  undefined clearBufferiv(GLenum buffer, GLint drawbuffer, Int32List values,
                          optional unsigned long long srcOffset = 0);
  undefined clearBufferuiv(GLenum buffer, GLint drawbuffer, Uint32List values,
                           optional unsigned long long srcOffset = 0);

  undefined clearBufferfi(GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil);

  /* Query Objects */
  WebGLQuery createQuery();
  undefined deleteQuery(WebGLQuery? query);
  [WebGLHandlesContextLoss] GLboolean isQuery(WebGLQuery? query);
  undefined beginQuery(GLenum target, WebGLQuery query);
  undefined endQuery(GLenum target);
  WebGLQuery? getQuery(GLenum target, GLenum pname);
  any getQueryParameter(WebGLQuery query, GLenum pname);

  /* Sampler Objects */
  WebGLSampler createSampler();
  undefined deleteSampler(WebGLSampler? sampler);
  [WebGLHandlesContextLoss] GLboolean isSampler(WebGLSampler? sampler);
  undefined bindSampler(GLuint unit, WebGLSampler? sampler);
  undefined samplerParameteri(WebGLSampler sampler, GLenum pname, GLint param);
  undefined samplerParameterf(WebGLSampler sampler, GLenum pname, GLfloat param);
  any getSamplerParameter(WebGLSampler sampler, GLenum pname);

  /* Sync objects */
  WebGLSync? fenceSync(GLenum condition, GLbitfield flags);
  [WebGLHandlesContextLoss] GLboolean isSync(WebGLSync? sync);
  undefined deleteSync(WebGLSync? sync);
  GLenum clientWaitSync(WebGLSync sync, GLbitfield flags, GLuint64 timeout);
  undefined waitSync(WebGLSync sync, GLbitfield flags, GLint64 timeout);
  any getSyncParameter(WebGLSync sync, GLenum pname);

  /* Transform Feedback */
  WebGLTransformFeedback createTransformFeedback();
  undefined deleteTransformFeedback(WebGLTransformFeedback? tf);
  [WebGLHandlesContextLoss] GLboolean isTransformFeedback(WebGLTransformFeedback? tf);
  undefined bindTransformFeedback (GLenum target, WebGLTransformFeedback? tf);
  undefined beginTransformFeedback(GLenum primitiveMode);
  undefined endTransformFeedback();
  undefined transformFeedbackVaryings(WebGLProgram program, sequence<DOMString> varyings, GLenum bufferMode);
  WebGLActiveInfo? getTransformFeedbackVarying(WebGLProgram program, GLuint index);
  undefined pauseTransformFeedback();
  undefined resumeTransformFeedback();

  /* Uniform Buffer Objects and Transform Feedback Buffers */
  undefined bindBufferBase(GLenum target, GLuint index, WebGLBuffer? buffer);
  undefined bindBufferRange(GLenum target, GLuint index, WebGLBuffer? buffer, GLintptr offset, GLsizeiptr size);
  any getIndexedParameter(GLenum target, GLuint index);
  sequence<GLuint>? getUniformIndices(WebGLProgram program, sequence<DOMString> uniformNames);
  any getActiveUniforms(WebGLProgram program, sequence<GLuint> uniformIndices, GLenum pname);
  GLuint getUniformBlockIndex(WebGLProgram program, DOMString uniformBlockName);
  any getActiveUniformBlockParameter(WebGLProgram program, GLuint uniformBlockIndex, GLenum pname);
  DOMString? getActiveUniformBlockName(WebGLProgram program, GLuint uniformBlockIndex);
  undefined uniformBlockBinding(WebGLProgram program, GLuint uniformBlockIndex, GLuint uniformBlockBinding);

  /* Vertex Array Objects */
  WebGLVertexArrayObject createVertexArray();
  undefined deleteVertexArray(WebGLVertexArrayObject? vertexArray);
  [WebGLHandlesContextLoss] GLboolean isVertexArray(WebGLVertexArrayObject? vertexArray);
  undefined bindVertexArray(WebGLVertexArrayObject? array);
};

interface mixin WebGL2RenderingContextOverloads
{
  // WebGL1:
  undefined bufferData(GLenum target, GLsizeiptr size, GLenum usage);
  undefined bufferData(GLenum target, AllowSharedBufferSource? srcData, GLenum usage);
  undefined bufferSubData(GLenum target, GLintptr dstByteOffset, AllowSharedBufferSource srcData);
  // WebGL2:
  undefined bufferData(GLenum target, [AllowShared] ArrayBufferView srcData, GLenum usage, unsigned long long srcOffset,
                       optional GLuint length = 0);
  undefined bufferSubData(GLenum target, GLintptr dstByteOffset, [AllowShared] ArrayBufferView srcData,
                          unsigned long long srcOffset, optional GLuint length = 0);

  // WebGL1 legacy entrypoints:
  undefined texImage2D(GLenum target, GLint level, GLint internalformat,
                       GLsizei width, GLsizei height, GLint border, GLenum format,
                       GLenum type, [AllowShared] ArrayBufferView? pixels);
  undefined texImage2D(GLenum target, GLint level, GLint internalformat,
                       GLenum format, GLenum type, TexImageSource source); // May throw DOMException

  undefined texSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                          GLsizei width, GLsizei height,
                          GLenum format, GLenum type, [AllowShared] ArrayBufferView? pixels);
  undefined texSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                          GLenum format, GLenum type, TexImageSource source); // May throw DOMException

  // WebGL2 entrypoints:
  undefined texImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height,
                       GLint border, GLenum format, GLenum type, GLintptr pboOffset);
  undefined texImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height,
                       GLint border, GLenum format, GLenum type,
                       TexImageSource source); // May throw DOMException
  undefined texImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height,
                       GLint border, GLenum format, GLenum type, [AllowShared] ArrayBufferView srcData,
                       unsigned long long srcOffset);

  undefined texSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width,
                          GLsizei height, GLenum format, GLenum type, GLintptr pboOffset);
  undefined texSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width,
                          GLsizei height, GLenum format, GLenum type,
                          TexImageSource source); // May throw DOMException
  undefined texSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width,
                          GLsizei height, GLenum format, GLenum type, [AllowShared] ArrayBufferView srcData,
                          unsigned long long srcOffset);

  undefined compressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width,
                                 GLsizei height, GLint border, GLsizei imageSize, GLintptr offset);
  undefined compressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width,
                                 GLsizei height, GLint border, [AllowShared] ArrayBufferView srcData,
                                 optional unsigned long long srcOffset = 0, optional GLuint srcLengthOverride = 0);

  undefined compressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                                    GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, GLintptr offset);
  undefined compressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                                    GLsizei width, GLsizei height, GLenum format,
                                    [AllowShared] ArrayBufferView srcData,
                                    optional unsigned long long srcOffset = 0,
                                    optional GLuint srcLengthOverride = 0);

  undefined uniform1fv(WebGLUniformLocation? location, Float32List data, optional unsigned long long srcOffset = 0,
                       optional GLuint srcLength = 0);
  undefined uniform2fv(WebGLUniformLocation? location, Float32List data, optional unsigned long long srcOffset = 0,
                       optional GLuint srcLength = 0);
  undefined uniform3fv(WebGLUniformLocation? location, Float32List data, optional unsigned long long srcOffset = 0,
                       optional GLuint srcLength = 0);
  undefined uniform4fv(WebGLUniformLocation? location, Float32List data, optional unsigned long long srcOffset = 0,
                       optional GLuint srcLength = 0);

  undefined uniform1iv(WebGLUniformLocation? location, Int32List data, optional unsigned long long srcOffset = 0,
                       optional GLuint srcLength = 0);
  undefined uniform2iv(WebGLUniformLocation? location, Int32List data, optional unsigned long long srcOffset = 0,
                       optional GLuint srcLength = 0);
  undefined uniform3iv(WebGLUniformLocation? location, Int32List data, optional unsigned long long srcOffset = 0,
                       optional GLuint srcLength = 0);
  undefined uniform4iv(WebGLUniformLocation? location, Int32List data, optional unsigned long long srcOffset = 0,
                       optional GLuint srcLength = 0);

  undefined uniformMatrix2fv(WebGLUniformLocation? location, GLboolean transpose, Float32List data,
                             optional unsigned long long srcOffset = 0, optional GLuint srcLength = 0);
  undefined uniformMatrix3fv(WebGLUniformLocation? location, GLboolean transpose, Float32List data,
                             optional unsigned long long srcOffset = 0, optional GLuint srcLength = 0);
  undefined uniformMatrix4fv(WebGLUniformLocation? location, GLboolean transpose, Float32List data,
                             optional unsigned long long srcOffset = 0, optional GLuint srcLength = 0);

  /* Reading back pixels */
  // WebGL1:
  undefined readPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type,
                       [AllowShared] ArrayBufferView? dstData);
  // WebGL2:
  undefined readPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type,
                       GLintptr offset);
  undefined readPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type,
                       [AllowShared] ArrayBufferView dstData, unsigned long long dstOffset);
};

[Exposed=(Window,Worker)]
interface WebGL2RenderingContext
{
};
WebGL2RenderingContext includes WebGLRenderingContextBase;
WebGL2RenderingContext includes WebGL2RenderingContextBase;
WebGL2RenderingContext includes WebGL2RenderingContextOverloads;
3.7.1 More binding points
void bindBuffer(GLenum target, WebGLBuffer? buffer) (OpenGL ES 3.0.6 §2.10.1, man page)
Binds the given WebGLBuffer object to the given binding point(target). target is given in the following table:
target
ARRAY_BUFFER
COPY_READ_BUFFER
COPY_WRITE_BUFFER
ELEMENT_ARRAY_BUFFER
PIXEL_PACK_BUFFER
PIXEL_UNPACK_BUFFER
TRANSFORM_FEEDBACK_BUFFER
UNIFORM_BUFFER
If target is not in the table above, generates an INVALID_ENUM error.

Refer to Buffer Object Binding restrictions below.
void bindFramebuffer(GLenum target, WebGLFramebuffer? framebuffer) (OpenGL ES 3.0.6 §4.4.1, man page)
Binds the given WebGLFramebuffer object to the given binding point(target). target is given in the following table:
target
FRAMEBUFFER
READ_FRAMEBUFFER
DRAW_FRAMEBUFFER
If target is not in the table above, generates an INVALID_ENUM error.

void bindTexture(GLenum target, WebGLTexture? texture) (OpenGL ES 3.0.6 §3.8.1, man page)
Binds the given WebGLTexture object to the given binding point(target). target is given in the following table:
target
TEXTURE_2D
TEXTURE_3D
TEXTURE_2D_ARRAY
TEXTURE_CUBE_MAP
If target is not in the table above, generates an INVALID_ENUM error.

3.7.2 Setting and getting state
any getParameter(GLenum pname) (OpenGL ES 3.0.6 §6.1.1, glGet OpenGL ES 3.0 man page, glGetString OpenGL ES 3.0 man page)
Return the value for the passed pname. As well as supporting all the pname/type values from WebGL 1.0, the following parameters are supported:
pname	returned type
COPY_READ_BUFFER_BINDING	WebGLBuffer
COPY_WRITE_BUFFER_BINDING	WebGLBuffer
DRAW_BUFFERi	GLenum
DRAW_FRAMEBUFFER_BINDING	WebGLFramebuffer
FRAGMENT_SHADER_DERIVATIVE_HINT	GLenum
MAX_3D_TEXTURE_SIZE	GLint
MAX_ARRAY_TEXTURE_LAYERS	GLint
MAX_CLIENT_WAIT_TIMEOUT_WEBGL	GLint64
MAX_COLOR_ATTACHMENTS	GLint
MAX_COMBINED_FRAGMENT_UNIFORM_COMPONENTS	GLint64
MAX_COMBINED_UNIFORM_BLOCKS	GLint
MAX_COMBINED_VERTEX_UNIFORM_COMPONENTS	GLint64
MAX_DRAW_BUFFERS	GLint
MAX_ELEMENT_INDEX	GLint64
MAX_ELEMENTS_INDICES	GLint
MAX_ELEMENTS_VERTICES	GLint
MAX_FRAGMENT_INPUT_COMPONENTS	GLint
MAX_FRAGMENT_UNIFORM_BLOCKS	GLint
MAX_FRAGMENT_UNIFORM_COMPONENTS	GLint
MAX_PROGRAM_TEXEL_OFFSET	GLint
MAX_SAMPLES	GLint
MAX_SERVER_WAIT_TIMEOUT	GLint64
MAX_TEXTURE_LOD_BIAS	GLfloat
MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS	GLint
MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS	GLint
MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS	GLint
MAX_UNIFORM_BLOCK_SIZE	GLint64
MAX_UNIFORM_BUFFER_BINDINGS	GLint
MAX_VARYING_COMPONENTS	GLint
MAX_VERTEX_OUTPUT_COMPONENTS	GLint
MAX_VERTEX_UNIFORM_BLOCKS	GLint
MAX_VERTEX_UNIFORM_COMPONENTS	GLint
MIN_PROGRAM_TEXEL_OFFSET	GLint
PACK_ROW_LENGTH	GLint
PACK_SKIP_PIXELS	GLint
PACK_SKIP_ROWS	GLint
PIXEL_PACK_BUFFER_BINDING	WebGLBuffer
PIXEL_UNPACK_BUFFER_BINDING	WebGLBuffer
RASTERIZER_DISCARD	GLboolean
READ_BUFFER	GLenum
READ_FRAMEBUFFER_BINDING	WebGLFramebuffer
SAMPLER_BINDING	WebGLSampler
TEXTURE_BINDING_2D_ARRAY	WebGLTexture
TEXTURE_BINDING_3D	WebGLTexture
TRANSFORM_FEEDBACK_ACTIVE	GLboolean
TRANSFORM_FEEDBACK_BINDING	WebGLTransformFeedback
TRANSFORM_FEEDBACK_BUFFER_BINDING	WebGLBuffer
TRANSFORM_FEEDBACK_PAUSED	GLboolean
UNIFORM_BUFFER_BINDING	WebGLBuffer
UNIFORM_BUFFER_OFFSET_ALIGNMENT	GLint
UNPACK_IMAGE_HEIGHT	GLint
UNPACK_ROW_LENGTH	GLint
UNPACK_SKIP_IMAGES	GLint
UNPACK_SKIP_PIXELS	GLint
UNPACK_SKIP_ROWS	GLint
VERTEX_ARRAY_BINDING	WebGLVertexArrayObject

All queries returning sequences or typed arrays return a new object each time.

If pname is not in the table above and is not one of parameter names supported by WebGL 1.0, generates an INVALID_ENUM error and returns null.

The following pname arguments return a string describing some aspect of the current WebGL implementation:

VERSION	Returns a version or release number of the form WebGL<space>2.0<optional><space><vendor-specific information></optional>.
SHADING_LANGUAGE_VERSION	Returns a version or release number of the form WebGL<space>GLSL<space>ES<space>3.00<optional><space><vendor-specific information></optional>.

For RED_BITS, GREEN_BITS, BLUE_BITS, and ALPHA_BITS, if active color attachments of the draw framebuffer do not have identical formats, generates an INVALID_OPERATION error and returns 0.

any getIndexedParameter(GLenum target, GLuint index) (OpenGL ES 3.0.6 §6.1.1, glGet OpenGL ES 3.0 man page)
Return the indexed value for the passed target. The type returned is the natural type for the requested pname, as given in the following table:
target	returned type
TRANSFORM_FEEDBACK_BUFFER_BINDING	WebGLBuffer
TRANSFORM_FEEDBACK_BUFFER_SIZE	GLsizeiptr
TRANSFORM_FEEDBACK_BUFFER_START	GLintptr
UNIFORM_BUFFER_BINDING	WebGLBuffer
UNIFORM_BUFFER_SIZE	GLsizeiptr
UNIFORM_BUFFER_START	GLintptr

If target is not in the table above, generates an INVALID_ENUM error.

If index is outside of the valid range for the indexed state target, generates an INVALID_VALUE error.

If an OpenGL error is generated, returns null.

GLboolean isEnabled(GLenum cap) (OpenGL ES 3.0.6 §6.1.1, OpenGL ES 3.0 man page)
In addition to all of the cap values from WebGL 1.0, RASTERIZER_DISCARD is supported.
void pixelStorei(GLenum pname, GLint param) (OpenGL ES 3.0.6 §3.7.1, OpenGL ES 3.0 man page)
In addition to the parameters from WebGL 1.0, the WebGL 2.0 specification accepts the following extra parameters:
pname
PACK_ROW_LENGTH
PACK_SKIP_PIXELS
PACK_SKIP_ROWS
UNPACK_ROW_LENGTH
UNPACK_IMAGE_HEIGHT
UNPACK_SKIP_PIXELS
UNPACK_SKIP_ROWS
UNPACK_SKIP_IMAGES

3.7.3 Buffer objects
void bufferData(GLenum target, [AllowShared] ArrayBufferView srcData, GLenum usage, unsigned long long srcOffset, optional GLuint length = 0); (OpenGL ES 3.0.6 §2.10.2, man page)
Set the size of the currently bound WebGLBuffer object, then copy a sub-region of srcData to the buffer object.

Let buf be the buffer bound to target.

If length is 0:
If srcData is a DataView, let copyLength be srcData.byteLength - srcOffset; the typed elements in the text below are bytes.
Otherwise, let copyLength be srcData.length - srcOffset.
Otherwise, let copyLength be length.

If srcData is a DataView, set the size of buf to copyLength; otherwise, set the size of buf to copyLength * srcData.BYTES_PER_ELEMENT.

If srcData is a DataView, let elementSize be 1; otherwise, let elementSize be srcData.BYTES_PER_ELEMENT.

If copyLength is greater than zero, copy copyLength typed elements (each of size elementSize) from srcData into buf, reading srcData starting at element index srcOffset. If copyLength is 0, no data is written to buf, but this does not cause a GL error to be generated.
If no WebGLBuffer is bound to target, generates an INVALID_OPERATION error.
If srcOffset is greater than srcData.length (or srcData.byteLength for DataView), generates an INVALID_VALUE error.
If srcOffset + copyLength is greater than srcData.length (or srcData.byteLength for DataView), generates an INVALID_VALUE error.
If any error is generated, buf's size is unmodified, and no data is written to it.
void bufferSubData(GLenum target, GLintptr dstByteOffset, [AllowShared] ArrayBufferView srcData, unsigned long long srcOffset, optional GLuint length = 0); (OpenGL ES 3.0.6 §2.10.2, man page)
Copy a sub-region of srcData to the currently bound WebGLBuffer object.

Let buf be the buffer bound to target.

If length is 0:
If srcData is a DataView, let copyLength be srcData.byteLength - srcOffset; the typed elements in the text below are bytes.
Otherwise, let copyLength be srcData.length - srcOffset.
Otherwise, let copyLength be length.

If srcData is a DataView, let copyByteLength be copyLength; otherwise, let copyByteLength be copyLength * srcData.BYTES_PER_ELEMENT.

If srcData is a DataView, let elementSize be 1; otherwise, let elementSize be srcData.BYTES_PER_ELEMENT.

If copyLength is greater than zero, copy copyLength typed elements (each of size elementSize) from srcData into buf, reading srcData starting at element index srcOffset, and writing buf starting at byte offset dstByteOffset. If copyLength is 0, no data is written to buf, but this does not cause a GL error to be generated.
If no WebGLBuffer is bound to target, generates an INVALID_OPERATION error.
If dstByteOffset is less than zero, generates an INVALID_VALUE error.
If dstByteOffset + copyByteLength is greater than the size of buf, generates an INVALID_VALUE error.
If srcOffset is greater than srcData.length (or srcData.byteLength for DataView), generates an INVALID_VALUE error.
If srcOffset + copyLength is greater than srcData.length (or srcData.byteLength for DataView), generates an INVALID_VALUE error.
If any error is generated, no data is written to buf.
any getBufferParameter(GLenum target, GLenum pname) (OpenGL ES 3.0.6 §6.1.9, man page)
Return the value for the passed pname. In addition to supporting querying with the pname BUFFER_USAGE as in WebGL 1.0, querying with the pname BUFFER_SIZE returns the buffer size as a value of type GLsizeiptr.
void copyBufferSubData(GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size) (OpenGL ES 3.0.6 §2.10.5, man page)

Copy part of the data of the buffer bound to readTarget to the buffer bound to writeTarget. See Copying Buffers for restrictions imposed by the WebGL 2.0 API.
undefined getBufferSubData(GLenum target, GLintptr srcByteOffset, [AllowShared] ArrayBufferView dstBuffer, optional unsigned long long dstOffset = 0, optional GLuint length = 0)

Reads back data from the bound WebGLBuffer into dstBuffer.

Let buf be the buffer bound to target.

If length is 0:
If dstBuffer is a DataView, let copyLength be dstBuffer.byteLength - dstOffset; the typed elements in the text below are bytes.
Otherwise, let copyLength be dstBuffer.length - dstOffset.
Otherwise, let copyLength be length.

If dstBuffer is a DataView, let copyByteLength be copyLength; otherwise, let copyByteLength be copyLength * dstBuffer.BYTES_PER_ELEMENT.

If dstBuffer is a DataView, let elementSize be 1; otherwise, let elementSize be dstBuffer.BYTES_PER_ELEMENT.

If copyLength is greater than zero, copy copyLength typed elements (each of size elementSize) from buf into dstBuffer, reading buf starting at byte index srcByteOffset and writing into dstBuffer starting at element index dstOffset. If copyLength is 0, no data is written to dstBuffer, but this does not cause a GL error to be generated.
If no WebGLBuffer is bound to target, generates an INVALID_OPERATION error.
If target is TRANSFORM_FEEDBACK_BUFFER, and any transform feedback object is currently active, generates an INVALID_OPERATION error.
If dstOffset is greater than dstBuffer.length (or dstBuffer.byteLength in the case of DataView), generates an INVALID_VALUE error.
If dstOffset + copyLength is greater than dstBuffer.length (or dstBuffer.byteLength in the case of DataView), generates an INVALID_VALUE error.
If srcByteOffset is less than zero, generates an INVALID_VALUE error.
If srcByteOffset + copyByteLength is greater than the size of buf, generates an INVALID_OPERATION error.
If any error is generated, no data is written to dstBuffer.

If the buffer is written and read sequentially by other operations and getBufferSubData, it is the responsibility of the WebGL API to ensure that data are accessed consistently. This applies even if the buffer is currently bound to a transform feedback binding point.
This is a blocking operation, as WebGL must completely finish all previous writes into the source buffer in order to return a result. In multi-process WebGL implementations, getBufferSubData can also incur an expensive inter-process round-trip to fetch the result from the remote process.

The user may be able to avoid these costs by signalling intent to read back from the buffer:

Insert a fenceSync after writing to the source buffer, and wait for it to pass before performing the getBufferSubData operation.
Allocate buffers with a _READ usage hint if they are to be used as a readback source. (Avoid overuse of buffers allocated with _READ usage hints, as they may incur overhead in maintaining a shadow copy of the buffer data.)
3.7.4 Framebuffer objects
[WebGLHandlesContextLoss] GLenum checkFramebufferStatus(GLenum target) (OpenGL ES 3.0.6 §4.4.4.2, man page)
Only differences from checkFramebufferStatus in WebGL 1.0 are described here.

target must be DRAW_FRAMEBUFFER, READ_FRAMEBUFFER or FRAMEBUFFER. FRAMEBUFFER is equivalent to DRAW_FRAMEBUFFER.

Returns FRAMEBUFFER_UNSUPPORTED if depth and stencil attachments, if present, are not the same image. See Framebuffer Object Attachments for detailed discussion.

Returns FRAMEBUFFER_INCOMPLETE_MULTISAMPLE if the values of RENDERBUFFER_SAMPLES are different among attached renderbuffers, or are non-zero if the attached images are a mix of renderbuffers and textures.

Returns FRAMEBUFFER_INCOMPLETE_DIMENSIONS if attached images have different width, height, and depth (for 3D textures) or array size (for 2D array textures). See checkFramebufferStatus may return FRAMEBUFFER_INCOMPLETE_DIMENSIONS.

Returns FRAMEBUFFER_UNSUPPORTED if the same image is attached to more than one color attachment point. See Framebuffer color attachments.

any getFramebufferAttachmentParameter(GLenum target, GLenum attachment, GLenum pname) (OpenGL ES 3.0.6 §6.1.13, similar to glGetFramebufferAttachmentParameteriv)
Return the value for the passed pname given the passed target and attachment. The type returned is the natural type for the requested pname, as given in the following table:
pname	returned type
FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE	GLint
FRAMEBUFFER_ATTACHMENT_BLUE_SIZE	GLint
FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING	GLenum
FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE	GLenum
FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE	GLint
FRAMEBUFFER_ATTACHMENT_GREEN_SIZE	GLint
FRAMEBUFFER_ATTACHMENT_OBJECT_NAME	WebGLRenderbuffer or WebGLTexture
FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE	GLenum
FRAMEBUFFER_ATTACHMENT_RED_SIZE	GLint
FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE	GLint
FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE	GLint
FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER	GLint
FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL	GLint
If pname is not in the table above, generates an INVALID_ENUM error.

If an OpenGL error is generated, returns null.

If attachment is DEPTH_STENCIL_ATTACHMENT and different images are attached to the depth and stencil attachment points, generates an INVALID_OPERATION error. See Framebuffer Object Attachments for detailed discussion.

void blitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) (OpenGL ES 3.0.6 §4.3.3, man page)

Both a [Read Operation] and a [Draw Operation].
Transfer a rectangle of pixel values from one region of the read framebuffer to another in the draw framebuffer. If the value of SAMPLE_BUFFERS for the read framebuffer is one and the value of SAMPLE_BUFFERS for the draw framebuffer is zero, the samples corresponding to each pixel location in the source are converted to a single sample before being written to the destination.

Any destination pixel who's center corresponds to a point outside the source buffer remain untouched.

When blitting to the color attachment of the WebGL context's default back buffer, a context created with alpha:false is considered to have internal format RGB8, while a context created with alpha:true is considered to have internal format RGBA8.

If this function attempts to blit to a missing attachment of a complete framebuffer, nothing is blitted to that attachment and no error is generated per Drawing to a Missing Attachment.
If this function attempts to read from a missing attachment of a complete framebuffer, and at least one draw buffer has an image to be blitted, an INVALID_OPERATION error is generated per Reading from a Missing Attachment.
void framebufferTextureLayer(GLenum target, GLenum attachment, WebGLTexture? texture, GLint level, GLint layer) (OpenGL ES 3.0.6 §4.4.2.4, man page)

If texture was generated by a different WebGL2RenderingContext than this one, generates an INVALID_OPERATION error.
void invalidateFramebuffer(GLenum target, sequence<GLenum> attachments) (OpenGL ES 3.0.6 §4.5, man page)

A [Draw Operation].
Equivalent to calling invalidateSubFramebuffer with x and y set to 0 and width and height set to the largest framebuffer object's attachments' width and height.
void invalidateSubFramebuffer (GLenum target, sequence<GLenum> attachments, GLint x, GLint y, GLsizei width, GLsizei height) (OpenGL ES 3.0.6 §4.5, man page)

A [Draw Operation].
Signal the GL that it need not preserve all contents of a bound framebuffer object.
void readBuffer(GLenum src) (OpenGL ES 3.0.6 §4.3.1, man page)

Specify a color buffer of the read framebuffer as the read buffer.
3.7.5 Renderbuffer objects
any getInternalformatParameter(GLenum target, GLenum internalformat, GLenum pname) (OpenGL ES 3.0.6 §6.1.15, man page)

Return the value for the passed pname given the passed target and internalformat. The type returned is given in the following table:
pname	returned type
SAMPLES	Int32Array
If pname is not in the table above, generates an INVALID_ENUM error.

If an OpenGL error is generated, returns null.

Each query for SAMPLES returns a new typed array object instance.

any getRenderbufferParameter(GLenum target, GLenum pname) (OpenGL ES 2.0 §6.1.14, similar to glGetRenderbufferParameteriv)
Return the value for the passed pname given the passed target. The type returned is the natural type for the requested pname, as given in the following table:
pname	returned type
RENDERBUFFER_WIDTH	GLint
RENDERBUFFER_HEIGHT	GLint
RENDERBUFFER_INTERNAL_FORMAT	GLenum
RENDERBUFFER_RED_SIZE	GLint
RENDERBUFFER_GREEN_SIZE	GLint
RENDERBUFFER_BLUE_SIZE	GLint
RENDERBUFFER_ALPHA_SIZE	GLint
RENDERBUFFER_DEPTH_SIZE	GLint
RENDERBUFFER_SAMPLES	GLint
RENDERBUFFER_STENCIL_SIZE	GLint
If pname is not in the table above, generates an INVALID_ENUM error.

If an OpenGL error is generated, returns null.

void renderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) (OpenGL ES 3.0.6 §4.4.2.1, man page)

Accepts internal formats from OpenGL ES 3.0 as detailed in the specification and man page.

To be backward compatible with WebGL 1, also accepts internal format DEPTH_STENCIL, which should be mapped to DEPTH24_STENCIL8 by implementations.

void renderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) (OpenGL ES 3.0.6 §4.4.2.1, man page)

Generates INVALID_OPERATION when `internalFormat == DEPTH_STENCIL && samples > 0`.

3.7.6 Texture objects
Texture objects provide storage and state for texturing operations. If no WebGLTexture is bound (e.g., passing null or 0 to bindTexture) then attempts to modify or query the texture object shall generate an INVALID_OPERATION error. This is indicated in the functions below in cases where The OpenGL ES 3.0 specification allows the function to change the default texture.

any getTexParameter(GLenum target, GLenum pname) (OpenGL ES 3.0.6 §6.1.3, man page)
Return the value for the passed pname given the passed target. The type returned is the natural type for the requested pname, as given in the following table:
pname	returned type
TEXTURE_BASE_LEVEL	GLint
TEXTURE_COMPARE_FUNC	GLenum
TEXTURE_COMPARE_MODE	GLenum
TEXTURE_IMMUTABLE_FORMAT	GLboolean
TEXTURE_IMMUTABLE_LEVELS	GLuint
TEXTURE_MAG_FILTER	GLenum
TEXTURE_MAX_LEVEL	GLint
TEXTURE_MAX_LOD	GLfloat
TEXTURE_MIN_FILTER	GLenum
TEXTURE_MIN_LOD	GLfloat
TEXTURE_WRAP_R	GLenum
TEXTURE_WRAP_S	GLenum
TEXTURE_WRAP_T	GLenum
If pname is not in the table above, generates an INVALID_ENUM error.

If an attempt is made to call this function with no WebGLTexture bound (see above), generates an INVALID_OPERATION error.

If an OpenGL error is generated, returns null.

void texParameterf(GLenum target, GLenum pname, GLfloat param) (OpenGL ES 3.0.6 §3.8.7, man page)
Set the value for the passed pname given the passed target. pname is given in the following table:
pname
TEXTURE_BASE_LEVEL
TEXTURE_COMPARE_FUNC
TEXTURE_COMPARE_MODE
TEXTURE_MAG_FILTER
TEXTURE_MAX_LEVEL
TEXTURE_MAX_LOD
TEXTURE_MIN_FILTER
TEXTURE_MIN_LOD
TEXTURE_WRAP_R
TEXTURE_WRAP_S
TEXTURE_WRAP_T
If pname is not in the table above, generates an INVALID_ENUM error.

If an attempt is made to call this function with no WebGLTexture bound (see above), generates an INVALID_OPERATION error.

void texParameteri(GLenum target, GLenum pname, GLint param) (OpenGL ES 3.0.6 §3.8.7, man page)
Set the value for the passed pname given the passed target. pname is this same with that of texParameterf, as given in the table above.

If pname is not in the table above, generates an INVALID_ENUM error.

If an attempt is made to call this function with no WebGLTexture bound (see above), generates an INVALID_OPERATION error.

void texStorage2D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) (OpenGL ES 3.0.6 §3.8.4, man page)

Specify all the levels of a two-dimensional or cube-map texture at the same time.

The image contents are set as if a buffer of sufficient size initialized to 0 would be passed to each texImage2D (or compressedTexImage2D for compressed formats) call in the pseudocode in The OpenGL ES 3.0 specification section 3.8.4 (OpenGL ES 3.0.6 §3.8.4).
texStorage2D should be considered a preferred alternative to texImage2D. It may have lower memory costs than texImage2D in some implementations.
void texStorage3D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth) (OpenGL ES 3.0.6 §3.8.4, man page)

Specify all the levels of a three-dimensional texture or two-dimensional array texture.

The image contents are set as if a buffer of sufficient size initialized to 0 would be passed to each texImage3D (or compressedTexImage3D for compressed formats) call in the pseudocode in The OpenGL ES 3.0 specification section 3.8.4 (OpenGL ES 3.0.6 §3.8.4).
undefined texImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, [AllowShared] ArrayBufferView srcData, unsigned long long srcOffset) (OpenGL ES 3.0.6 §3.8.3, man page)
Only differences from texImage2D in WebGL 1.0 are described here.

If a WebGLBuffer is bound to the PIXEL_UNPACK_BUFFER target, generates an INVALID_OPERATION error.

Sized internal formats are supported in WebGL 2.0 and internalformat is no longer required to be the same as format. Instead, the combination of internalformat, format, and type must be listed in Table 1 or 2 from man page.

If type is specified as FLOAT_32_UNSIGNED_INT_24_8_REV, srcData must be null; otherwise, generates an INVALID_OPERATION error.

The type of srcData must match the type according to the following table; otherwise, generates an INVALID_OPERATION error:

type of srcData	type
Int8Array	BYTE
Uint8Array	UNSIGNED_BYTE
Uint8ClampedArray	UNSIGNED_BYTE
Int16Array	SHORT
Uint16Array	UNSIGNED_SHORT
Uint16Array	UNSIGNED_SHORT_5_6_5
Uint16Array	UNSIGNED_SHORT_5_5_5_1
Uint16Array	UNSIGNED_SHORT_4_4_4_4
Int32Array	INT
Uint32Array	UNSIGNED_INT
Uint32Array	UNSIGNED_INT_5_9_9_9_REV
Uint32Array	UNSIGNED_INT_2_10_10_10_REV
Uint32Array	UNSIGNED_INT_10F_11F_11F_REV
Uint32Array	UNSIGNED_INT_24_8
Uint16Array	HALF_FLOAT
Float32Array	FLOAT
If pixel store parameter constraints are not met, generates an INVALID_OPERATION error.

Reading from srcData begins srcOffset elements into srcData. (Elements are bytes for Uint8Array, int32s for Int32Array, etc.)

If there's not enough data in srcData starting at srcOffset, generate INVALID_OPERATION.

[throws] undefined texImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, TexImageSource source) // May throw DOMException (OpenGL ES 3.0.6 §3.8.3, man page)
Only differences from texImage2D in WebGL 1.0 are described here.

Conversion to new formats introduced in WebGL 2.0 is performed according to the following table.

Source DOM Image Format	Target WebGL Format
RED	RG
Grayscale (1 channel)	R = sourceGray
R = sourceGray
G = 0
Grayscale + Alpha (2 channels)	R = sourceGray
R = sourceGray
G = 0
Color (3 channels)
Color + Alpha (4 channels)	R = sourceRed
R = sourceRed
G = sourceGreen

Uploading subregions of elements is detailed in Pixel store parameters for uploads from TexImageSource.

If a WebGLBuffer is bound to the PIXEL_UNPACK_BUFFER target, generates an INVALID_OPERATION error.

Sized internal formats are supported in WebGL 2.0 and internalformat is no longer required to be the same as format. Instead, the combination of internalformat, format, and type must be listed in the following table:

Internal Format	Format	Type
RGB	RGB	UNSIGNED_BYTE
UNSIGNED_SHORT_5_6_5
RGBA	RGBA	UNSIGNED_BYTE,
UNSIGNED_SHORT_4_4_4_4
UNSIGNED_SHORT_5_5_5_1
LUMINANCE_ALPHA	LUMINANCE_ALPHA	UNSIGNED_BYTE
LUMINANCE	LUMINANCE	UNSIGNED_BYTE
ALPHA	ALPHA	UNSIGNED_BYTE
R8	RED	UNSIGNED_BYTE
R16F	RED	HALF_FLOAT
FLOAT
R32F	RED	FLOAT
R8UI	RED_INTEGER	UNSIGNED_BYTE
RG8	RG	UNSIGNED_BYTE
RG16F	RG	HALF_FLOAT
FLOAT
RG32F	RG	FLOAT
RG8UI	RG_INTEGER	UNSIGNED_BYTE
RGB8	RGB	UNSIGNED_BYTE
SRGB8	RGB	UNSIGNED_BYTE
RGB565	RGB	UNSIGNED_BYTE
UNSIGNED_SHORT_5_6_5
R11F_G11F_B10F	RGB	UNSIGNED_INT_10F_11F_11F_REV
HALF_FLOAT
FLOAT
RGB9_E5	RGB	HALF_FLOAT
FLOAT
RGB16F	RGB	HALF_FLOAT
FLOAT
RGB32F	RGB	FLOAT
RGB8UI	RGB_INTEGER	UNSIGNED_BYTE
RGBA8	RGBA	UNSIGNED_BYTE
SRGB8_ALPHA8	RGBA	UNSIGNED_BYTE
RGB5_A1	RGBA	UNSIGNED_BYTE
UNSIGNED_SHORT_5_5_5_1
RGB10_A2	RGBA	UNSIGNED_INT_2_10_10_10_REV
RGBA4	RGBA	UNSIGNED_BYTE
UNSIGNED_SHORT_4_4_4_4
RGBA16F	RGBA	HALF_FLOAT
FLOAT
RGBA32F	RGBA	FLOAT
RGBA8UI	RGBA_INTEGER	UNSIGNED_BYTE
When the data source is a DOM element (HTMLImageElement, HTMLCanvasElement, or HTMLVideoElement), or is an ImageBitmap, ImageData, or OffscreenCanvas object, commonly each channel's representation is an unsigned integer type of at least 8 bits. Converting such representation to signed integers or unsigned integers with more bits is not clearly defined. For example, when converting RGBA8 to RGBA16UI, it is unclear whether or not the intention is to scale up values to the full range of a 16-bit unsigned integer. Therefore, only converting to unsigned integer of at most 8 bits, half float, or float is allowed.
void texImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, GLintptr offset) (OpenGL ES 3.0.6 §3.8.3, man page)
Upload data to the currently bound WebGLTexture from the WebGLBuffer bound to the PIXEL_UNPACK_BUFFER target.

offset is the byte offset into the WebGLBuffer's data store; generates an INVALID_VALUE if it's less than 0.

The combination of format, type, and WebGLTexture's internal format must be listed in Table 1 or 2 from man page.

If an attempt is made to call the function with no WebGLTexture bound, generates an INVALID_OPERATION error.

If no WebGLBuffer is bound to the PIXEL_UNPACK_BUFFER target, generates an INVALID_OPERATION error.

If pixel store parameter constraints are not met, generates an INVALID_OPERATION error.

undefined texSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, [AllowShared] ArrayBufferView srcData, unsigned long long srcOffset) (OpenGL ES 3.0.6 §3.8.5, man page)
Only differences from texSubImage2D in WebGL 1.0 are described here.

If a WebGLBuffer is bound to the PIXEL_UNPACK_BUFFER target, generates an INVALID_OPERATION error.

The combination of format, type, and WebGLTexture's internal format must be listed in Table 1 or 2 from man page.

The type of srcData must match the type according to the above table; otherwise, generates an INVALID_OPERATION error.

See Pixel Storage Parameters for WebGL-specific pixel storage parameters that affect the behavior of this function.

If pixel store parameter constraints are not met, generates an INVALID_OPERATION error.

Reading from srcData begins srcOffset elements into srcData. (Elements are bytes for Uint8Array, int32s for Int32Array, etc.)

If there's not enough data in srcData starting at srcOffset, generate INVALID_OPERATION.

undefined texSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, TexImageSource source) // May throw DOMException (OpenGL ES 3.0.6 §3.8.5, man page)

Only differences from texSubImage2D in WebGL 1.0 are described here.

Uploading subregions of elements is detailed in Pixel store parameters for uploads from TexImageSource.

If a WebGLBuffer is bound to the PIXEL_UNPACK_BUFFER target, generates an INVALID_OPERATION error.

The combination of format, type, and WebGLTexture's internal format must be listed in this table.

undefined texSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, GLintptr offset) (OpenGL ES 3.0.6 §3.8.5, man page)

Updates a sub-rectangle of the currently bound WebGLTexture with data from the WebGLBuffer bound to PIXEL_UNPACK_BUFFER target.

offset is the byte offset into the WebGLBuffer's data store; generates an INVALID_VALUE error if it's less than 0.

If an attempt is made to call the function with no WebGLTexture bound, generates an INVALID_OPERATION error.

If no WebGLBuffer is bound to the PIXEL_UNPACK_BUFFER target, generates an INVALID_OPERATION error.

If pixel store parameter constraints are not met, generates an INVALID_OPERATION error.

void texImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, [AllowShared] ArrayBufferView? srcData)

void texImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, [AllowShared] ArrayBufferView srcData, unsigned long long srcOffset) (OpenGL ES 3.0.6 §3.8.3, man page)

Allocates and initializes the specified mipmap level of a three-dimensional or two-dimensional array texture.

If a WebGLBuffer is bound to the PIXEL_UNPACK_BUFFER target, generates an INVALID_OPERATION error.

If srcData is null, a buffer of sufficient size initialized to 0 is passed.

The combination of internalformat, format, and type must be listed in Table 1 or 2 from man page.

If type is specified as FLOAT_32_UNSIGNED_INT_24_8_REV, srcData must be null; otherwise, generates an INVALID_OPERATION error.

If srcData is non-null, the type of srcData must match the type according to the above table; otherwise, generate an INVALID_OPERATION error.

If an attempt is made to call this function with no WebGLTexture bound (see above), generates an INVALID_OPERATION error.

See Pixel Storage Parameters for WebGL-specific pixel storage parameters that affect the behavior of this function.

If pixel store parameter constraints are not met, generates an INVALID_OPERATION error.

Reading from srcData begins srcOffset elements into srcData. (Elements are bytes for Uint8Array, int32s for Int32Array, etc.)

If there's not enough data in srcData starting at srcOffset, generate INVALID_OPERATION.

It is recommended to use texStorage3D instead of texImage3D to allocate three-dimensional textures. texImage3D may impose a higher memory cost compared to texStorage3D in some implementations.
undefined texImage3D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, TexImageSource source) // May throw DOMException (OpenGL ES 3.0.6 §3.8.3, man page)

Update a rectangular subregion of the currently bound WebGLTexture.

Uploading subregions of elements is detailed in Pixel store parameters for uploads from TexImageSource.

See texImage2D for the interpretation of the format and type arguments, and notes on the UNPACK_PREMULTIPLY_ALPHA_WEBGL pixel storage parameter.

See Pixel Storage Parameters for WebGL-specific pixel storage parameters that affect the behavior of this function when it is called with any argument type other than ImageBitmap.

The first pixel transferred from the source to the WebGL implementation corresponds to the upper left corner of the source. This behavior is modified by the UNPACK_FLIP_Y_WEBGL pixel storage parameter, except for ImageBitmap arguments, as described in the abovementioned section.

The combination of format, type, and WebGLTexture's internal format must be listed in this table.

If an attempt is made to call this function with no WebGLTexture bound (see above), generates an INVALID_OPERATION error.

If a WebGLBuffer is bound to the PIXEL_UNPACK_BUFFER target, generates an INVALID_OPERATION error.

If this function is called with an ImageData whose data attribute has been neutered, an INVALID_VALUE error is generated.

If this function is called with an ImageBitmap that has been neutered, an INVALID_VALUE error is generated.

If this function is called with an HTMLImageElement or HTMLVideoElement whose origin differs from the origin of the containing Document, or with an HTMLCanvasElement, ImageBitmap, or OffscreenCanvas whose bitmap's origin-clean flag is set to false, a SECURITY_ERR exception must be thrown. See Origin Restrictions.

void texImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, GLintptr offset) (OpenGL ES 3.0.6 §3.8.3, man page)
Upload data to the currently bound WebGLTexture from the WebGLBuffer bound to the PIXEL_UNPACK_BUFFER target.

offset is the byte offset into the WebGLBuffer's data store; generates an INVALID_VALUE error if it's less than 0.

If an attempt is made to call the function with no WebGLTexture bound, generates an INVALID_OPERATION error.

If no WebGLBuffer is bound to the PIXEL_UNPACK_BUFFER target, generates an INVALID_OPERATION error.

If pixel store parameter constraints are not met, generates an INVALID_OPERATION error.

undefined texSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, [AllowShared] ArrayBufferView? srcData, optional unsigned long long srcOffset = 0) (OpenGL ES 3.0.6 §3.8.5, man page)

Update a rectangular subregion of the currently bound WebGLTexture.

If an attempt is made to call this function with no WebGLTexture bound (see above), generates an INVALID_OPERATION error.

If a WebGLBuffer is bound to the PIXEL_UNPACK_BUFFER target, generates an INVALID_OPERATION error.

The combination of format, type, and WebGLTexture's internal format must be listed in Table 1 or 2 from man page.

If type is FLOAT_32_UNSIGNED_INT_24_8_REV, generates an INVALID_ENUM error.

The type of srcData must match the type according to the above table; otherwise, generates an INVALID_OPERATION error.

If srcData is non-null but its size is less than what is required by the specified width, height, depth, format, type, and pixel storage parameters, generates an INVALID_OPERATION error.

See Pixel Storage Parameters for WebGL-specific pixel storage parameters that affect the behavior of this function.

If pixel store parameter constraints are not met, generates an INVALID_OPERATION error.

Reading from srcData begins srcOffset elements into srcData. (Elements are bytes for Uint8Array, int32s for Int32Array, etc.)

If there's not enough data in srcData starting at srcOffset, generate INVALID_OPERATION.

undefined texSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, TexImageSource source) // May throw DOMException (OpenGL ES 3.0.6 §3.8.5, man page)

Update a rectangular subregion of the currently bound WebGLTexture.

Uploading subregions of elements is detailed in Pixel store parameters for uploads from TexImageSource.

See texImage2D for the interpretation of the format and type arguments, and notes on the UNPACK_PREMULTIPLY_ALPHA_WEBGL pixel storage parameter.

See Pixel Storage Parameters for WebGL-specific pixel storage parameters that affect the behavior of this function when it is called with any argument type other than ImageBitmap.

The first pixel transferred from the source to the WebGL implementation corresponds to the upper left corner of the source. This behavior is modified by the UNPACK_FLIP_Y_WEBGL pixel storage parameter, except for ImageBitmap arguments, as described in the abovementioned section.

The combination of format, type, and WebGLTexture's internal format must be listed in this table.

If an attempt is made to call this function with no WebGLTexture bound (see above), generates an INVALID_OPERATION error.

If a WebGLBuffer is bound to the PIXEL_UNPACK_BUFFER target, generates an INVALID_OPERATION error.

If this function is called with an ImageData whose data attribute has been neutered, an INVALID_VALUE error is generated.

If this function is called with an ImageBitmap that has been neutered, an INVALID_VALUE error is generated.

If this function is called with an HTMLImageElement or HTMLVideoElement whose origin differs from the origin of the containing Document, or with an HTMLCanvasElement, ImageBitmap, or OffscreenCanvas whose bitmap's origin-clean flag is set to false, a SECURITY_ERR exception must be thrown. See Origin Restrictions.

void texSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, GLintptr offset) (OpenGL ES 3.0.6 §3.8.5, man page)

Updates a sub-rectangle of the currently bound WebGLTexture with data from the WebGLBuffer bound to PIXEL_UNPACK_BUFFER target.

offset is the byte offset into the WebGLBuffer's data store; generates an INVALID_VALUE error if it's less than 0.

If an attempt is made to call the function with no WebGLTexture bound, generates an INVALID_OPERATION error.

If no WebGLBuffer is bound to the PIXEL_UNPACK_BUFFER target, generates an INVALID_OPERATION error.

If pixel store parameter constraints are not met, generates an INVALID_OPERATION error.

void copyTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height) (OpenGL ES 3.0.6 §3.8.5, man page)

A [Read Operation].
If an attempt is made to call this function with no WebGLTexture bound (see above), an INVALID_OPERATION error is generated.

For any pixel lying outside the framebuffer, the corresponding destination pixel remains untouched; see Reading Pixels Outside the Framebuffer.

If this function attempts to read from a missing attachment of a complete framebuffer, an INVALID_OPERATION error is generated per Reading from a Missing Attachment.
undefined compressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, [AllowShared] ArrayBufferView srcData, optional unsigned long long srcOffset = 0, optional GLuint srcLengthOverride = 0) (OpenGL ES 3.0.6 §3.8.6, man page)

Reading from srcData begins srcOffset elements into srcData. (Elements are bytes for Uint8Array, int32s for Int32Array, etc.)

If srcOffset > srcData.length, generates an INVALID_VALUE error.

srcLengthOverride defaults to srcData.length - srcOffset.

If there's not enough data in srcData starting at srcOffset, or if the amount of data passed in is not consistent with the format, dimensions, and contents of the compressed image, generates an INVALID_VALUE error.

undefined compressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, [AllowShared] ArrayBufferView srcData, optional unsigned long long srcOffset = 0, optional GLuint srcLengthOverride = 0) (OpenGL ES 3.0.6 §3.8.6, man page)

Reading from srcData begins srcOffset elements into srcData. (Elements are bytes for Uint8Array, int32s for Int32Array, etc.)

If srcOffset > srcData.length, generates an INVALID_VALUE error.

srcLengthOverride defaults to srcData.length - srcOffset.

If there's not enough data in srcData starting at srcOffset, or if the amount of data passed in is not consistent with the format, dimensions, and contents of the compressed image, generates an INVALID_VALUE error.

undefined compressedTexImage3D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, [AllowShared] ArrayBufferView srcData, optional unsigned long long srcOffset = 0, optional GLuint srcLengthOverride = 0) (OpenGL ES 3.0.6 §3.8.6, man page)

Reading from srcData begins srcOffset elements into srcData. (Elements are bytes for Uint8Array, int32s for Int32Array, etc.)

If srcOffset > srcData.length, generates an INVALID_VALUE error.

srcLengthOverride defaults to srcData.length - srcOffset.

If there's not enough data in srcData starting at srcOffset, or if the amount of data passed in is not consistent with the format, dimensions, and contents of the compressed image, generates an INVALID_VALUE error.

undefined compressedTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, [AllowShared] ArrayBufferView srcData, optional unsigned long long srcOffset = 0, optional GLuint srcLengthOverride = 0) (OpenGL ES 3.0.6 §3.8.6, man page)

Reading from srcData begins srcOffset elements into srcData. (Elements are bytes for Uint8Array, int32s for Int32Array, etc.)

If srcOffset > srcData.length, generates an INVALID_VALUE error.

srcLengthOverride defaults to srcData.length - srcOffset.

If there's not enough data in srcData starting at srcOffset, or if the amount of data passed in is not consistent with the format, dimensions, and contents of the compressed image, generates an INVALID_VALUE error.

This section applies to the above four entry points.

If an attempt is made to call these functions with no WebGLTexture bound (see above), generates an INVALID_OPERATION error.

If a WebGLBuffer is bound to the PIXEL_UNPACK_BUFFER target, generates an INVALID_OPERATION error.

The ETC2 and EAC texture formats defined in OpenGL ES 3.0 are not available in WebGL 2.0.

void compressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, GLintptr offset) (OpenGL ES 3.0.6 §3.8.6, man page)

void compressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, GLintptr offset) (OpenGL ES 3.0.6 §3.8.6, man page)

void compressedTexImage3D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, GLintptr offset) (OpenGL ES 3.0.6 §3.8.6, man page)

void compressedTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, GLintptr offset) (OpenGL ES 3.0.6 §3.8.6, man page)

This section applies to the above four entry points.

If an attempt is made to call these functions with no WebGLTexture bound (see above), generates an INVALID_OPERATION error.

If no WebGLBuffer is bound to the PIXEL_UNPACK_BUFFER target, generates an INVALID_OPERATION error.

offset is the byte offset into the WebGLBuffer's data store; generates an INVALID_VALUE error if it's less than 0.

The ETC2 and EAC texture formats defined in OpenGL ES 3.0 are not available in WebGL 2.0.

3.7.7 Programs and Shaders
[WebGLHandlesContextLoss] GLint getFragDataLocation(WebGLProgram program, DOMString name) (OpenGL ES 3.0.6 §3.9.2.3, man page)

If program was generated by a different WebGL2RenderingContext than this one, generates an INVALID_OPERATION error and returns -1.
any getProgramParameter(WebGLProgram? program, GLenum pname) (OpenGL ES 3.0.6 §6.1.12, man page)
If program was generated by a different WebGL2RenderingContext than this one, generates an INVALID_OPERATION error and returns null.

Return the value for the passed pname given the passed program. The type returned is the natural type for the requested pname, as given in the following table:
pname	returned type
DELETE_STATUS	GLboolean
LINK_STATUS	GLboolean
VALIDATE_STATUS	GLboolean
ATTACHED_SHADERS	GLint
ACTIVE_ATTRIBUTES	GLint
ACTIVE_UNIFORMS	GLint
TRANSFORM_FEEDBACK_BUFFER_MODE	GLenum
TRANSFORM_FEEDBACK_VARYINGS	GLint
ACTIVE_UNIFORM_BLOCKS	GLint
If pname is not in the table above, generates an INVALID_ENUM error and returns null.

Returns null if any OpenGL errors are generated during the execution of this function.

3.7.8 Uniforms and attributes
any getUniform(WebGLProgram program, WebGLUniformLocation location) (OpenGL ES 3.0.6 §6.1.12, man page)
If either program or location were generated by a different WebGL2RenderingContext than this one, generates an INVALID_OPERATION error.

Return the uniform value at the passed location in the passed program. The type returned is dependent on the uniform type. The types returned for the new uniform types in WebGL 2.0 are given in the following table:
uniform type	returned type
uint	GLuint
uvec2	Uint32Array (with 2 elements)
uvec3	Uint32Array (with 3 elements)
uvec4	Uint32Array (with 4 elements)
mat2x3	Float32Array (with 6 elements)
mat2x4	Float32Array (with 8 elements)
mat3x2	Float32Array (with 6 elements)
mat3x4	Float32Array (with 12 elements)
mat4x2	Float32Array (with 8 elements)
mat4x3	Float32Array (with 12 elements)
any sampler type	GLint

The types returned for the uniform types shared with WebGL 1.0 are the same as in WebGL 1.0.

void uniform[1234]ui(WebGLUniformLocation? location, ...)

void uniform[1234]uiv(WebGLUniformLocation? location, ...)

void uniformMatrix[234]x[234]fv(WebGLUniformLocation? location, ...) (OpenGL ES 3.0.6 §2.12.6, man page)

Each of the uniform* functions above sets the specified uniform or uniforms to the values provided. If the passed location is not null and was not obtained from the currently used program via an earlier call to getUniformLocation, an INVALID_OPERATION error will be generated. If the passed location is null, the data passed in will be silently ignored and no uniform variables will be changed.

If the array passed to any of the vector forms (those ending in v) has an invalid length, an INVALID_VALUE error will be generated. The length is invalid if it is too short for or is not an integer multiple of the assigned type.
In overloads with a srcLength arg:

If srcLength is 0, it defaults to data.length - srcOffset.
If srcOffset + srcLength is longer than data.length, generate INVALID_VALUE.

void vertexAttribI4[u]i(GLuint index, ...)

void vertexAttribI4[u]iv(GLuint index, ...) (OpenGL ES 3.0.6 §2.8, man page)

Sets the vertex attribute at the passed index to the given constant integer value. Values set via the vertexAttrib are guaranteed to be returned from the getVertexAttrib function with the CURRENT_VERTEX_ATTRIB param, even if there have been intervening calls to drawArrays or drawElements.

If the array passed to any of the vector forms (those ending in v) is too short, an INVALID_VALUE error will be generated.
void vertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride, GLintptr offset) (OpenGL ES 3.0.6 §2.9, man page)

Assign the WebGLBuffer object currently bound to the ARRAY_BUFFER target to the vertex attribute at the passed index. Values are always left as integer values. Size is number of components per attribute. Stride and offset are in units of bytes. Passed stride and offset must be appropriate for the passed type and size or an INVALID_OPERATION error will be generated; see Buffer Offset and Stride Requirements. If offset is negative, an INVALID_VALUE error will be generated. If no WebGLBuffer is bound to the ARRAY_BUFFER target and offset is non-zero, an INVALID_OPERATION error will be generated. In WebGL, the maximum supported stride is 255; see Vertex Attribute Data Stride.
any getVertexAttrib(GLuint index, GLenum pname) (OpenGL ES 3.0.6 §6.1.12, man page)
Return the information requested in pname about the vertex attribute at the passed index. The type returned is dependent on the information requested, as shown in the following table:
pname	returned type
VERTEX_ATTRIB_ARRAY_BUFFER_BINDING	WebGLBuffer
VERTEX_ATTRIB_ARRAY_ENABLED	GLboolean
VERTEX_ATTRIB_ARRAY_SIZE	GLint
VERTEX_ATTRIB_ARRAY_STRIDE	GLint
VERTEX_ATTRIB_ARRAY_TYPE	GLenum
VERTEX_ATTRIB_ARRAY_NORMALIZED	GLboolean
CURRENT_VERTEX_ATTRIB	One of Float32Array, Int32Array or Uint32Array (each with 4 elements)
VERTEX_ATTRIB_ARRAY_INTEGER	GLboolean
VERTEX_ATTRIB_ARRAY_DIVISOR	GLint
For CURRENT_VERTEX_ATTRIB, the return type is dictated by the most recent call to the vertexAttrib family of functions for the given index. That is, if vertexAttribI4i* was used, the return type will be Int32Array; If vertexAttribI4ui* was used, the return type will be Uint32Array; Otherwise, Float32Array.

All queries returning sequences or typed arrays return a new object each time.

If pname is not in the table above, generates an INVALID_ENUM error.

If an OpenGL error is generated, returns null.

3.7.9 Writing to the drawing buffer
void clear(GLbitfield mask) (OpenGL ES 3.0.6 §4.2.3, man page)

Clear buffers to preset values. If an integer color buffer is among the buffers that would be cleared, an INVALID_OPERATION error is generated and nothing is cleared.
void vertexAttribDivisor(GLuint index, GLuint divisor) (OpenGL ES 3.0.6 §2.9, man page)

Set the rate at which the vertex attribute identified by index advances when drawing.
void drawArrays(GLenum mode, GLint first, GLsizei count) (OpenGL ES 3.0.6 §2.9.3, man page)

A [Draw Operation].
void drawElements(GLenum mode, GLsizei count, GLenum type, GLintptr offset) (OpenGL ES 3.0.6 §2.9.3, man page)

A [Draw Operation].
void drawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei instanceCount) (OpenGL ES 3.0.6 §2.9.3, man page)

A [Draw Operation].
Draw instanceCount instances of geometry using the currently enabled vertex attributes. Vertex attributes which have a non-zero divisor advance once every divisor instances.

void drawElementsInstanced(GLenum mode, GLsizei count, GLenum type, GLintptr offset, GLsizei instanceCount) (OpenGL ES 3.0.6 §2.9.3, man page)

A [Draw Operation].
Draw instanceCount instances of geometry using the currently bound element array buffer. Vertex attributes which have a non-zero divisor advance once every divisor instances.

void drawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, GLintptr offset) (OpenGL ES 3.0.6 §2.9.3, man page)

A [Draw Operation].
Draw using the currently bound element array buffer. All error conditions specified for drawElements in the section Writing to the drawing buffer of the WebGL 1.0 specification apply.

In addition, if indices used to draw are outside the range of [start, end], an implementation can either guarantee the behaviors defined in Enabled Vertex Attributes and Range Checking, or simply discard the arguments start and end and call drawElements instead. In either situation, no GL errors should be generated for this cause.
During calls to drawElements, drawArrays, drawRangeElements and their instanced variants, WebGL 2.0 performs additional error checking beyond that specified in OpenGL ES 3.0:

If the CURRENT_PROGRAM is null, an INVALID_OPERATION error will be generated;
Range Checking;
Active Uniform Block Backing;
VertexAttrib function must match shader attribute type.
3.7.10 Reading back pixels
Pixels in the current framebuffer can be read back into an ArrayBufferView object or a WebGLBuffer bound to the PIXEL_PACK_BUFFER target.

Only differences from Reading back pixels in WebGL 1.0 are described here.

undefined readPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, [AllowShared] ArrayBufferView dstData, unsigned long long dstOffset) (OpenGL ES 3.0 §4.3.2, man page)
A [Read Operation].
If a WebGLBuffer is bound to the PIXEL_PACK_BUFFER target, generates an INVALID_OPERATION error.

If pixel store parameter constraints are not met, generates an INVALID_OPERATION error.

If dstData doesn't have enough space for the read operation starting at dstOffset, generate INVALID_OPERATION.

This is a blocking operation, as WebGL must completely finish all previous rendering operations into the source framebuffer in order to return a result. In multi-process WebGL implementations, it also incurs an expensive inter-process round-trip to fetch the result from the remote process.

Consider instead using readPixels into a PIXEL_PACK_BUFFER. Use getBufferSubData to read the data from that buffer. (See getBufferSubData for how to avoid blocking in that call.)

void readPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLintptr offset) (OpenGL ES 3.0 §4.3.2, man page)
If no WebGLBuffer is bound to the PIXEL_PACK_BUFFER target, generates an INVALID_OPERATION error.

offset is the byte offset into the WebGLBuffer's data store; generates an INVALID_VALUE error if it's less than 0. If the remainder of the WebGLBuffer's data store is not large enough to retrieve all of the pixels in the specified rectangle taking into account pixel store modes, generates an INVALID_OPERATION.

3.7.11 Multiple render targets
void drawBuffers(sequence<GLenum> buffers) (OpenGL ES 3.0.6 §4.2.1, man page)

Define the draw buffers to which all fragment colors are written.
This does not draw, this only sets up state for subsequent calls.
void clearBufferfv(GLenum buffer, GLint drawbuffer, Float32List values, optional unsigned long long srcOffset = 0);

void clearBufferiv(GLenum buffer, GLint drawbuffer, Int32List values, optional unsigned long long srcOffset = 0);

void clearBufferuiv(GLenum buffer, GLint drawbuffer, Uint32List values, optional unsigned long long srcOffset = 0);

void clearBufferfi(GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil); (OpenGL ES 3.0.6 §4.2.3, man page) These are [Draw Operations].

Set every pixel in the specified buffer to a constant value. The clearBuffer function that should be used for a color buffer depends on the type of the color buffer, given in the following table:
Type of buffer	clearBuffer function
floating point	clearBufferfv
fixed point	clearBufferfv
signed integer	clearBufferiv
unsigned integer	clearBufferuiv

If buffer is COLOR_BUFFER and the function is not chosen according to the above table, an INVALID_OPERATION error is generated and nothing is cleared.
For ArrayBufferView entrypoints, if there's not enough elements in values starting at srcOffset, generate INVALID_VALUE.

clearBufferfi may be used to clear the depth and stencil buffers. buffer must be DEPTH_STENCIL and drawBuffer must be zero. depth and stencil are the depth and stencil values, respectively.

If this function attempts to clear a missing attachment of a complete framebuffer, nothing is cleared and no error is generated per Drawing to a Missing Attachment.

3.7.12 Query objects
WebGLQuery? createQuery() (OpenGL ES 3.0.6 §2.14, man page)

Create a WebGLQuery object and initialize it with a query object name as if by calling glGenQueries.
void deleteQuery(WebGLQuery? query) (OpenGL ES 3.0.6 §2.14, man page)

If query was generated by a different WebGL2RenderingContext than this one, generates an INVALID_OPERATION error.

Mark for deletion the query object contained in the passed WebGLQuery, as if by calling glDeleteQueries. If the object has already been marked for deletion, the call has no effect. Note that underlying GL object will be automatically marked for deletion when the JS object is destroyed, however this method allows authors to mark an object for deletion early.
[WebGLHandlesContextLoss] GLboolean isQuery(WebGLQuery? query) (OpenGL ES 3.0.6 §6.1.7, man page)

Return true if the passed WebGLQuery is valid and false otherwise.

Returns false if the query was generated by a different WebGL2RenderingContext than this one.

Returns false if the query's invalidated flag is set.
void beginQuery(GLenum target, WebGLQuery query) (OpenGL ES 3.0.6 §2.14, man page)

If query was generated by a different WebGL2RenderingContext than this one, generates an INVALID_OPERATION error.

Begin an asynchronous query. Target indicates the type of query to be performed.
void endQuery(GLenum target) (OpenGL ES 3.0.6 §2.14, man page)

Mark the end of the sequence of commands to be tracked for the query type given by target. When the final query result is available, the query object is updated to indicate this and the result may be retrieved by calling getQueryParameter.
WebGLQuery? getQuery(GLenum target, GLenum pname) (OpenGL ES 3.0.6 §6.1.7, man page)

Returns information about a query target target, which must be one of ANY_SAMPLES_PASSED or ANY_SAMPLES_PASSED_CONSERVATIVE for occlusion queries, or TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN for primitive queries. pname specifies the symbolic name of a query object target parameter. Currently it must be CURRENT_QUERY, and returns either the currently active query for the target, or null.

If target or pname are not in the list above, generates an INVALID_ENUM error and returns null.

Returns null if any OpenGL errors are generated during the execution of this function.

any getQueryParameter(WebGLQuery query, GLenum pname) (OpenGL ES 3.0.6 §6.1.7, man page)

If query was generated by a different WebGL2RenderingContext than this one, generates an INVALID_OPERATION error.

Returns a parameter pname of a query object. QUERY_RESULT returns the value of the query object's passed samples counter. QUERY_RESULT_AVAILABLE returns whether the samples counter is immediately available. The type returned is the natural type for the requested pname, as given in the following table:

pname	returned type
QUERY_RESULT	GLuint
QUERY_RESULT_AVAILABLE	GLboolean
If pname is not in the table above, generates an INVALID_ENUM error and returns null.

If query is not a valid query object, or is a currently active query object, generates an INVALID_OPERATION error and returns null.

Returns null if any OpenGL errors are generated during the execution of this function.

In order to ensure consistent behavior across platforms, queries' results must only be made available when the user agent's event loop is not executing a task. In other words:

A query's result must not be made available until control has returned to the user agent's main loop.
Repeatedly fetching a query's QUERY_RESULT_AVAILABLE parameter in a loop, without returning control to the user agent, must always return the same value.
A query's result may or may not be made available when control returns to the user agent's event loop. It is not guaranteed that using a single setTimeout callback with a delay of 0, or a single requestAnimationFrame callback, will allow sufficient time for the WebGL implementation to supply the query's results.
This change compared to the OpenGL ES 3.0 specification is enforced in order to prevent applications from relying on being able to issue a query and fetch its result in the same frame. In order to ensure best portability among devices and best performance among implementations, applications must expect that queries' results will become available asynchronously.
3.7.13 Sampler objects
WebGLSampler? createSampler() (OpenGL ES 3.0.6 §3.8.2, man page)

Create a WebGLSampler object and initialize it with a sampler object name as if by calling glGenSamplers.
void deleteSampler(WebGLSampler? sampler) (OpenGL ES 3.0.6 §3.8.2, man page)

If sampler was generated by a different WebGL2RenderingContext than this one, generates an INVALID_OPERATION error.

Mark for deletion the sampler object contained in the passed WebGLSampler, as if by calling glDeleteSamplers. If the object has already been marked for deletion, the call has no effect. Note that underlying GL object will be automatically marked for deletion when the JS object is destroyed, however this method allows authors to mark an object for deletion early.
[WebGLHandlesContextLoss] GLboolean isSampler(WebGLSampler? sampler) (OpenGL ES 3.0.6 §6.1.5, man page)

Return true if the passed WebGLSampler is valid and false otherwise.

Returns false if the sampler was generated by a different WebGL2RenderingContext than this one.

Returns false if the sampler's invalidated flag is set.
void bindSampler(GLuint unit, WebGLSampler? sampler) (OpenGL ES 3.0.6 §3.8.2, man page)

If sampler was generated by a different WebGL2RenderingContext than this one, generates an INVALID_OPERATION error.

Bind the sampler object contained in the passed WebGLSampler to the texture unit at the passed index. If a sampler is bound to a texture unit, the sampler's state supersedes the sampling state of the texture bound to that texture unit. If sampler is null, the currently bound sampler is unbound from the texture unit. A single sampler object may be bound to multiple texture units simultaneously.

If unit is greater than or equal to the value of MAX_COMBINED_TEXTURE_IMAGE_UNITS, generates an INVALID_VALUE error.

An attempt to bind an object marked for deletion will generate an INVALID_OPERATION error, and the current binding will remain untouched.
void samplerParameteri(WebGLSampler sampler, GLenum pname, GLint param)

void samplerParameterf(WebGLSampler sampler, GLenum pname, GLfloat param)

(OpenGL ES 3.0.6 §3.8.2, man page)
If sampler was generated by a different WebGL2RenderingContext than this one, generates an INVALID_OPERATION error.

Set the value for the passed pname given the passed sampler. pname is given in the following table:
pname
TEXTURE_COMPARE_FUNC
TEXTURE_COMPARE_MODE
TEXTURE_MAG_FILTER
TEXTURE_MAX_LOD
TEXTURE_MIN_FILTER
TEXTURE_MIN_LOD
TEXTURE_WRAP_R
TEXTURE_WRAP_S
TEXTURE_WRAP_T
If pname is not in the table above, generates an INVALID_ENUM error.

If sampler is not a valid sampler object, generates an INVALID_OPERATION error.

any getSamplerParameter(WebGLSampler sampler, GLenum pname) (OpenGL ES 3.0.6 §6.1.5, man page)
If sampler was generated by a different WebGL2RenderingContext than this one, generates an INVALID_OPERATION error.

Return the information requested in pname about the given WebGLSampler, passed as sampler. The type returned is dependent on the information requested, as shown in the following table:
pname	returned type
TEXTURE_COMPARE_FUNC	GLenum
TEXTURE_COMPARE_MODE	GLenum
TEXTURE_MAG_FILTER	GLenum
TEXTURE_MAX_LOD	GLfloat
TEXTURE_MIN_FILTER	GLenum
TEXTURE_MIN_LOD	GLfloat
TEXTURE_WRAP_R	GLenum
TEXTURE_WRAP_S	GLenum
TEXTURE_WRAP_T	GLenum
If pname is not in the table above, generates an INVALID_ENUM error.

If sampler is not a valid sampler object, generates an INVALID_OPERATION error.

Returns null if any OpenGL errors are generated during the execution of this function.

3.7.14 Sync objects
Sync objects can be used to synchronize execution between the GL server and the client.

WebGLSync? fenceSync(GLenum condition, GLbitfield flags) (OpenGL ES 3.0.6 §5.2, man page)

Create a new fence sync object and insert an associated fence command in the GL command stream.
[WebGLHandlesContextLoss] GLboolean isSync(WebGLSync? sync) (OpenGL ES 3.0.6 §6.1.8, man page)

Return true if the passed WebGLSync is valid and false otherwise.

Returns false if the sync was generated by a different WebGL2RenderingContext than this one.

Returns false if the sync's invalidated flag is set.
void deleteSync(WebGLSync? sync) (OpenGL ES 3.0.6 §5.2, man page)

If sync was generated by a different WebGL2RenderingContext than this one, generates an INVALID_OPERATION error.

Mark for deletion the sync object contained in the passed WebGLSync, as if by calling glDeleteSync. If the object has already been marked for deletion, the call has no effect. Note that underlying GL object will be automatically marked for deletion when the JS object is destroyed, however this method allows authors to mark an object for deletion early.
GLenum clientWaitSync(WebGLSync sync, GLbitfield flags, GLuint64 timeout) (OpenGL ES 3.0.6 §5.2.1, man page)

If sync was generated by a different WebGL2RenderingContext than this one, generates an INVALID_OPERATION error.

Block execution until the passed sync object is signaled or the specified timeout has passed. timeout is in units of nanoseconds.

Returns one of four status values. A return value of ALREADY_SIGNALED indicates that sync was signaled at the time clientWaitSync was called. ALREADY_SIGNALED will always be returned if sync was signaled, even if timeout was zero. A return value of TIMEOUT_EXPIRED indicates that the specified timeout period expired before sync was signaled. A return value of CONDITION_SATISFIED indicates that sync was signaled before the timeout expired. Finally, if an error occurs, in addition to generating an error as specified below, returns WAIT_FAILED without blocking.

flags controls command flushing behavior and may include SYNC_FLUSH_COMMANDS_BIT. If any other bit is set in flags an INVALID_OPERATION error is generated. If SYNC_FLUSH_COMMANDS_BIT is set in flags and sync is unsignaled when clientWaitSync is called, then the equivalent of flush will be performed before blocking on sync.

As discussed in the differences section, WebGL implementations must impose a short maximum timeout to prevent blocking the main thread for long periods of time. The implementation-defined timeout may be queried by calling getParameter with the argument MAX_CLIENT_WAIT_TIMEOUT_WEBGL. If timeout is larger than this implementation-defined timeout then an INVALID_OPERATION error is generated.

The implementation-defined maximum timeout is not specified. It should be set low enough to keep applications from compromising interactivity by waiting for long periods of time. It is acceptable for an implementation to impose a zero maximum timeout. WebGL applications should not use clientWaitSync to block execution for long periods of time.
Returns WAIT_FAILED if any OpenGL errors are generated during the execution of this function.

In order to ensure consistent behavior across platforms, sync objects may only transition to the signaled state when the user agent's event loop is not executing a task. In other words:

clientWaitSync must not return CONDITION_SATISFIED or ALREADY_SIGNALED for a newly created sync object until control has returned to the user agent's main loop.
void waitSync(WebGLSync sync, GLbitfield flags, GLint64 timeout) (OpenGL ES 3.0.6 §5.2.1, man page)

If sync was generated by a different WebGL2RenderingContext than this one, generates an INVALID_OPERATION error.

Return immediately, but wait on the GL server until the passed sync object is signaled or an implementation-dependent timeout has passed. The passed timeout must be set to TIMEOUT_IGNORED.
In the absence of the possibility of synchronizing between multiple GL contexts, calling waitSync is effectively a no-op.
any getSyncParameter(WebGLSync sync, GLenum pname) (OpenGL ES 3.0.6 §6.1.8, man page)
If sync was generated by a different WebGL2RenderingContext than this one, generates an INVALID_OPERATION error.

Return the value for the passed pname given the passed WebGLSync object. The type returned is the natural type for the requested pname, as given in the following table:
pname	returned type
OBJECT_TYPE	GLenum
SYNC_STATUS	GLenum
SYNC_CONDITION	GLenum
SYNC_FLAGS	GLbitfield
If pname is not in the table above, generates an INVALID_ENUM error and returns null.

Returns null if any OpenGL errors are generated during the execution of this function.

In order to ensure consistent behavior across platforms, sync objects may only transition to the signaled state when the user agent's event loop is not executing a task. In other words:

A sync object must not become signaled until control has returned to the user agent's main loop.
Repeatedly fetching a sync object's SYNC_STATUS parameter in a loop, without returning control to the user agent, must always return the same value.
3.7.15 Transform feedback
Transform feedback mode captures the values of output variables written by the vertex shader. The vertices are captured before flatshading and clipping. The transformed vertices may be optionally discarded after being stored into one or more buffer objects, or they can be passed on down to the clipping stage for further processing. The set of output variables captured is determined when a program is linked.

If any output variable is specified to be streamed to a transform feedback buffer object but not actually written by a vertex shader, the value is set to 0. See Transform feedback primitive capture.

WebGLTransformFeedback? createTransformFeedback() (OpenGL ES 3.0.6 §2.15.1, similar to glGenTransformFeedbacks)
Create a WebGLTransformFeedback object and initialize it with a transform feedback object name as if by calling glGenTransformFeedbacks.
void deleteTransformFeedback(WebGLTransformFeedback? transformFeedback) (OpenGL ES 3.0.6 §2.15.1, similar to glDeleteTransformFeedbacks)
If transformFeedback was generated by a different WebGL2RenderingContext than this one, generates an INVALID_OPERATION error.

Mark for deletion the transform feedback object contained in the passed WebGLTransformFeedback, as if by calling glDeleteTransformFeedbacks. If the object has already been marked for deletion, the call has no effect. Note that underlying GL object will be automatically marked for deletion when the JS object is destroyed, however this method allows authors to mark an object for deletion early.
[WebGLHandlesContextLoss] GLboolean isTransformFeedback(WebGLTransformFeedback? transformFeedback) (OpenGL ES 3.0.6 §6.1.11, man page)
Return true if the passed WebGLTransformFeedback is valid and false otherwise.

Returns false if the transform feedback was generated by a different WebGL2RenderingContext than this one.

Returns false if the transform feedback's invalidated flag is set.
void bindTransformFeedback (GLenum target, WebGLTransformFeedback? transformFeedback) (OpenGL ES 3.0.6 §2.15.1, man page)
If transformFeedback was generated by a different WebGL2RenderingContext than this one, generates an INVALID_OPERATION error.

Bind the given WebGLTransformFeedback object. If transformFeedback is null, the default transform feedback object provided by the context is bound. An attempt to bind an object marked for deletion will generate an INVALID_OPERATION error, and the current binding will remain untouched.
void beginTransformFeedback(GLenum primitiveMode) (OpenGL ES 3.0.6 §2.15.2, man page)
void endTransformFeedback() (OpenGL ES 3.0.6 §2.15.2, man page)
void pauseTransformFeedback() (OpenGL ES 3.0.6 §2.15.2, man page)
void resumeTransformFeedback() (OpenGL ES 3.0.6 §2.15.2, man page)
void transformFeedbackVaryings(WebGLProgram program, sequence<DOMString> varyings, GLenum bufferMode) (OpenGL ES 3.0.6 §2.12.8, man page)
If program was generated by a different WebGL2RenderingContext than this one, generates an INVALID_OPERATION error.

WebGLActiveInfo? getTransformFeedbackVarying(WebGLProgram program, GLuint index) (OpenGL ES 3.0.6 §2.12.8, man page)
If program was generated by a different WebGL2RenderingContext than this one, generates an INVALID_OPERATION error and returns null.

3.7.16 Uniform Buffer objects
Uniform buffer objects provide the storage for named uniform blocks, so the values of active uniforms in named uniform blocks may be changed by modifying the contents of the buffer object.

void bindBufferBase(GLenum target, GLuint index, WebGLBuffer? buffer) (OpenGL ES 3.0.6 §2.10.1.1, man page)
If buffer was generated by a different WebGL2RenderingContext than this one, generates an INVALID_OPERATION error.

Binds the given WebGLBuffer object to the binding point at index of the array of targets specified by target.
void bindBufferRange(GLenum target, GLuint index, WebGLBuffer? buffer, GLintptr offset, GLsizeiptr size) (OpenGL ES 3.0.6 §2.10.1.1, man page)
If buffer was generated by a different WebGL2RenderingContext than this one, generates an INVALID_OPERATION error.

Binds a range of the WebGLBuffer object buffer represented by offset and size to the binding point at index of the array of targets specified by target.
sequence<GLuint>? getUniformIndices(WebGLProgram program, sequence<DOMString> uniformNames) (OpenGL ES 3.0.6 §2.12.6, man page)
If program was generated by a different WebGL2RenderingContext than this one, generates an INVALID_OPERATION error.

Retrieves the indices of a number of uniforms within program.

Returns null if any OpenGL errors are generated during the execution of this function.
any getActiveUniforms(WebGLProgram program, sequence<GLuint> uniformIndices, GLenum pname) (OpenGL ES 3.0.6 §2.12.6, man page)
If program was generated by a different WebGL2RenderingContext than this one, generates an INVALID_OPERATION error.

Queries the value of the parameter named pname for each of the uniforms within program whose indices are specified in the array of uniformIndices. The type returned is the natural type for the requested pname, as given in the following table:
pname	returned type
UNIFORM_TYPE	sequence<GLenum>
UNIFORM_SIZE	sequence<GLuint>
UNIFORM_BLOCK_INDEX	sequence<GLint>
UNIFORM_OFFSET	sequence<GLint>
UNIFORM_ARRAY_STRIDE	sequence<GLint>
UNIFORM_MATRIX_STRIDE	sequence<GLint>
UNIFORM_IS_ROW_MAJOR	sequence<GLboolean>

If pname is not in the table above, generates an INVALID_ENUM error.

Returns null if any OpenGL errors are generated during the execution of this function.

GLuint getUniformBlockIndex(WebGLProgram program, DOMString uniformBlockName) (OpenGL ES 3.0.6 §2.12.6, man page)
If program was generated by a different WebGL2RenderingContext than this one, generates an INVALID_OPERATION error.

Retrieves the index of a uniform block within program.
any getActiveUniformBlockParameter(WebGLProgram program, GLuint uniformBlockIndex, GLenum pname) (OpenGL ES 3.0.6 §2.12.6, man page)
If program was generated by a different WebGL2RenderingContext than this one, generates an INVALID_OPERATION error.

Retrieves information about an active uniform block within program. The type returned is the natural type for the requested pname, as given in the following table:
pname	returned type
UNIFORM_BLOCK_BINDING	GLuint
UNIFORM_BLOCK_DATA_SIZE	GLuint
UNIFORM_BLOCK_ACTIVE_UNIFORMS	GLuint
UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES	Uint32Array
UNIFORM_BLOCK_REFERENCED_BY_VERTEX_SHADER	GLboolean
UNIFORM_BLOCK_REFERENCED_BY_FRAGMENT_SHADER	GLboolean

If pname is not in the table above, generates an INVALID_ENUM error.

If uniformBlockIndex is not an active block uniform for program or greater than or equal to the value of ACTIVE_UNIFORM_BLOCKS, generates an INVALID_VALUE error.

If an OpenGL error is generated, returns null.

DOMString? getActiveUniformBlockName(WebGLProgram program, GLuint uniformBlockIndex) (OpenGL ES 3.0.6 §2.12.6, man page)
If program was generated by a different WebGL2RenderingContext than this one, generates an INVALID_OPERATION error.

Retrieves the name of the active uniform block at uniformBlockIndex within program.
void uniformBlockBinding(WebGLProgram program, GLuint uniformBlockIndex, GLuint uniformBlockBinding) (OpenGL ES 3.0.6 §2.12.6.5, man page)
If program was generated by a different WebGL2RenderingContext than this one, generates an INVALID_OPERATION error.

Assigns binding points for active uniform blocks.
3.7.17 Vertex Array objects
Vertex Array objects (sometimes referred to as VAOs) encapsulate all state related to the definition of data used by the vertex processor.

void bindVertexArray(WebGLVertexArrayObject? vertexArray) (OpenGL ES 3.0.6 §2.11, man page)
If vertexArray was generated by a different WebGL2RenderingContext than this one, generates an INVALID_OPERATION error.

Bind the given WebGLVertexArrayObject object. If vertexArray is null, the default vertex array provided by the context is bound. An attempt to bind a deleted vertex array will generate a INVALID_OPERATION error, and the current binding will remain untouched.
WebGLVertexArrayObject? createVertexArray() (OpenGL ES 3.0.6 §2.11, similar to glGenVertexArrays)
Create a WebGLVertexArrayObject object and initialize it with a vertex array object name as if by calling glGenVertexArrays.
void deleteVertexArray(WebGLVertexArrayObject? vertexArray) (OpenGL ES 3.0.6 §2.11, similar to glDeleteVertexArrays)
If vertexArray was generated by a different WebGL2RenderingContext than this one, generates an INVALID_OPERATION error.

Mark for deletion the vertex array object contained in the passed WebGLVertexArrayObject, as if by calling glDeleteVertexArrays. If the object has already been marked for deletion, the call has no effect. Note that underlying GL object will be automatically marked for deletion when the JS object is destroyed, however this method allows authors to mark an object for deletion early.
[WebGLHandlesContextLoss] GLboolean isVertexArray(WebGLVertexArrayObject? vertexArray) (OpenGL ES 3.0.6 §6.1.10, man page)
Return true if the passed WebGLVertexArrayObject is valid and false otherwise.

Returns false if the vertex array was generated by a different WebGL2RenderingContext than this one.

Returns false if the vertex array's invalidated flag is set.
4 Other differences Between WebGL 2.0 and WebGL 1.0
Needs update for WebGL 2.0
4.1 Backwards Incompatibility
4.1.1 Errors
The WebGL 2.0 API may behave differently in cases where the WebGL 1.0 API generates an error. Code written against the WebGL 1.0 API that generates errors is not guaranteed to be forward-compatible with WebGL 2.0.

4.1.2 Extensions
Some extensions that may have been supported in the WebGL 1.0 API are removed from the WebGL 2.0 API. For more details, see the WebGL Extension Registry.

Extensions are typically removed only if equivalent functionality is available in the WebGL 2.0 API either in the core specification or in an improved extension. When an application using WebGL 1.0 extensions is modified to run on the WebGL 2.0 API, it is often possible to create a dummy extension object for each of the promoted extensions which simply redirects calls to the appropriate WebGL 2.0 API functions. If the application is using shader language extensions, porting shaders to GLSL ES 3.00 is typically required.
4.1.3 Non-Power-of-Two Texture Access
Texture access works in the WebGL 2.0 API as in the OpenGL ES 3.0 API. In other words, unlike the WebGL 1.0 API, there are no special restrictions on non power of 2 textures. All mipmapping and all wrapping modes are supported for non-power-of-two images.

4.1.4 Primitive Restart is Always Enabled
See section PRIMITIVE_RESTART_FIXED_INDEX is always enabled.

4.1.5 Framebuffer Object Attachments
In WebGL 1.0, DEPTH_STENCIL_ATTACHMENT is an alternative attachment point other than DEPTH_ATTACHMENT and STENCIL_ATTACHMENT. In WebGL 2.0, however, DEPTH_STENCIL_ATTACHMENT is considered an alias for DEPTH_ATTACHMENT + STENCIL_ATTACHMENT, i.e., the same image is attached to both DEPTH_ATTACHMENT and STENCIL_ATTACHMENT, overwriting the original images attached to the two attachment points.


Consider the following sequence of actions:

attach renderbuffer 1 to DEPTH_ATTACHMENT;
attach renderbuffer 2 to DEPTH_STENCIL_ATTACHMENT;
attach null to DEPTH_STENCIL_ATTACHMENT.
In WebGL 1.0, the framebuffer ends up with renderbuffer 1 attached to DEPTH_ATTACHMENT and no image attached to STENCIL_ATTACHMENT; in WebGL 2.0, however, neither DEPTH_ATTACHMENT nor STENCIL_ATTACHMENT has an image attached.
The constraints defined in Framebuffer Object Attachments no longer apply in WebGL 2.0.

If different images are bound to the depth and stencil attachment points, checkFramebufferStatus returns FRAMEBUFFER_UNSUPPORTED, and getFramebufferAttachmentParameter with attachment of DEPTH_STENCIL_ATTACHMENT generates an INVALID_OPERATION error.

4.1.6 Texture Type in TexSubImage2D Calls
In the WebGL 1.0 API, the type argument passed to texSubImage2D must match the type used to originally define the texture object (i.e., using texImage2D). In the WebGL 2.0 API, this restriction has been lifted.

4.1.7 Out-of-bounds Behaviors in copyTexSubImage2D and readPixels calls
In WebGL 1.0, Reading Pixels Outside the Framebuffer, it is required that copyTexSubImage2D and readPixels do not touch the corresponding destination range for out-of-bound pixels.

In WebGL 2.0, when a PACK_BUFFER object is bound and PACK_ROW_LENGTH is not zero and less than width, or when a UNPACK_BUFFER object is bound and UNPACK_ROW_LENGTH is not zero and less than width or UNPACK_IMAGE_HEIGHT is not zero and less than height, packing/unpacking a row/image may extend to the next row/image. If that portion is out-of-bounds, the values may change accordingly.

4.1.8 Color conversion in copyTex{Sub}Image2D
In WebGL 1.0 (OpenGL ES 2.0), it is allowed for the component sizes of internalformat to be less than the corresponding component sizes of the source buffer's internal format. However, in WebGL 2.0 (OpenGL ES 3.0), if internalformat is sized, its component sizes must exactly match the corresponding component sizes of the source buffer's effective internal format.

In both WebGL 1.0 and 2.0, source buffer components can be dropped during the conversion to internalformat, but new components cannot be added.

4.1.9 getFramebufferAttachmentParameter with FRAMEBUFFER_ATTACHMENT_OBJECT_NAME
If getFramebufferAttachmentParameter is called with pname FRAMEBUFFER_ATTACHMENT_OBJECT_NAME and attachment has no image attached, in WebGL 1.0 / ES 2.0, it generates an INVALID_OPERATION; in WebGL 2.0 / ES 3.0, it generates no error and null is returned.

4.2 New Features Supported in the WebGL 2.0 API
Pixel buffer objects (OpenGL ES 3.0.6 §3.7.1 and OpenGL ES 3.0.6 §4.3)
Primitive restart (OpenGL ES 3.0.6 §2.9.1)
Rasterizer discard (OpenGL ES 3.0.6 §3.1)
4.3 GLSL ES 3.00 support
In addition to supporting The OpenGL ES Shading Language, Version 1.00, the WebGL 2.0 API also accepts shaders written in The OpenGL ES Shading Language, Version 3.00 [GLES30GLSL], with some restrictions.

A shader referencing state variables or functions that are available in other versions of GLSL, such as that found in versions of OpenGL for the desktop, must not be allowed to load.
As in the WebGL 1.0 API, identifiers starting with "webgl_" and "_webgl_" are reserved for use by WebGL. A shader which declares a function, variable, structure name, or structure field starting with these prefixes must not be allowed to load.

4.3.1 Maximum GLSL Token Size
WebGL 1.0 supports tokens up to 256 characters in length. WebGL 2.0 follows The OpenGL ES Shading Language, Version 3.00 (OpenGL ES 3.0.6 §1.5.1) and allows tokens up to 1024 characters in length for both ESSL 1 and ESSL 3 shaders. Shaders containing tokens longer than 1024 characters must fail to compile.

4.3.2 Maximum Uniform and Attribute Location Lengths
WebGL 2.0 imposes a limit of 1024 characters on the lengths of uniform and attribute locations.

4.4 Vertex Attribute Divisor
In the WebGL 2.0 API, vertex attributes which have a non-zero divisor do not advance during calls to drawArrays and drawElements. (OpenGL ES 3.0.6 §2.9.3)

5 Differences Between WebGL and OpenGL ES 3.0
This section describes changes made to the WebGL API relative to the OpenGL ES 3.0 API to improve portability across various operating systems and devices.

5.1 Buffer Object Binding
WebGL buffer type	Binding points that set this type
undefined	none
element array	ELEMENT_ARRAY_BUFFER
other data	all binding points except ELEMENT_ARRAY_BUFFER, COPY_READ_BUFFER and COPY_WRITE_BUFFER

In the WebGL 2.0 API, buffers have their WebGL buffer type initially set to undefined. Calling bindBuffer, bindBufferRange or bindBufferBase with the target argument set to any buffer binding point except COPY_READ_BUFFER or COPY_WRITE_BUFFER will then set the WebGL buffer type of the buffer being bound according to the table above. Binding a buffer with type undefined to the COPY_READ_BUFFER or COPY_WRITE_BUFFER binding points will set its type to other data.

Any call to one of these functions which attempts to bind a deleted buffer will generate a INVALID_OPERATION error, and the binding will remain untouched.

Any call to one of these functions which attempts to bind a WebGLBuffer that has the element array WebGL buffer type to a binding point that falls under other data, or bind a WebGLBuffer which has the other data WebGL buffer type to ELEMENT_ARRAY_BUFFER will generate an INVALID_OPERATION error, and the state of the binding point will remain untouched.

This restriction implies that a given buffer object may contain either indices or other data, but not both.

These restrictions are similar to buffer object binding restrictions in the WebGL 1.0 specification.

This restriction has been added to prevent writing to index buffers on the GPU, which would make doing any CPU-side checks on index data prohibitively expensive. Handling index buffers as different from other buffer data also maps better to the Direct3D API.
5.2 Copying Buffers
Attempting to use copyBufferSubData to copy between buffers that have element array and other data WebGL buffer types as specified in section Buffer Object Binding generates an INVALID_OPERATION error and no copying is performed.

Same as with Buffer Object Binding restrictions above.
5.3 Preventing undefined behavior with Transform Feedback
A buffer which is simultaneously bound to an indexed TRANSFORM_FEEDBACK_BUFFER binding point in the currently bound transform feedback object and any other binding point in the WebGL API, except generic TRANSFORM_FEEDBACK_BUFFER binding point, cannot be used. Any attempted use of such a double bound buffer fails with an INVALID_OPERATION error, regardless of whether transform feedback is enabled. For example, readPixels to a PIXEL_PACK_BUFFER will fail if the buffer is also bound to the current transform feedback object.

If an error were not generated, the values read or written would be undefined. ( OpenGL ES 3.0.6 §2.15.2)
In case the same buffer is bound to more than one indexed binding point in the active transform feedback, beginTransformFeedback generates an INVALID_OPERATION error.

This can happen only in SEPARATE_ATTRIBS mode.
This is a restriction from D3D11, where writing two different streams to the same buffer is not allowed.
5.4 Draw Buffers
The value of the MAX_COLOR_ATTACHMENTS parameter must be equal to that of the MAX_DRAW_BUFFERS parameter.

There is no use case for these parameters being different.
If an ESSL1 fragment shader writes to neither gl_FragColor nor gl_FragData, the values of the fragment colors following shader execution are untouched. If corresponding output variables are not defined in an ESSL3 fragment shader, the values of the fragment colors following shader execution are untouched.

All user-defined output variables default to zero if they are not written during a shader execution.

For optimal performance, an output array should not include any elements that are not accessed.
A draw buffer is "shader-output-incompatible" with a fragment shader if:

the values written to it by the shader do not match the format of the corresponding color buffer (For example, the output variable is an integer, but the corresponding color buffer has a floating-point format, or vice versa)
OR there is no defined fragment shader output for its location.
If any draw buffers are "shader-output-incompatible", draws generate INVALID_OPERATION if:

Any channel of colorMask is set to true
AND, for each DRAW_BUFFERi of DRAW_FRAMEBUFFER that is "shader-output-incompatible":
DRAW_BUFFERi is not NONE
AND, any channel colorMaski (from e.g. OES_draw_buffers_indexed) is set to true.
5.5 No Program Binaries
Accessing binary representations of compiled shader programs is not supported in the WebGL 2.0 API. This includes OpenGL ES 3.0 GetProgramBinary, ProgramBinary, and ProgramParameteri entry points. In addition, querying the program binary length with getProgramParameter, and querying program binary formats with getParameter are not supported in the WebGL 2.0 API.

5.6 Range Checking
In addition to the range checking specified in the WebGL 1.0 specification section Enabled Vertex Attributes and Range Checking, indices referenced by drawElements, drawRangeElements or drawElementsInstanced that are greater than the MAX_ELEMENT_INDEX parameter cause the draw call to generate an INVALID_OPERATION error even if they lie within the storage of the bound buffer. Range checking is not performed for indices that trigger primitive restart if primitive restart is enabled. The range checking specified for drawArrays in the WebGL 1.0 API is also applied to drawArraysInstanced in the WebGL 2.0 API.

The OpenGL robustness extensions do not specify what happens if indices above MAX_ELEMENT_INDEX are used, so passing them to the driver is risky. On platforms where MAX_ELEMENT_INDEX is the same as the maximum unsigned integer value this section will have no effect.
5.7 Active Uniform Block Backing
In the WebGL 2.0 API, attempting to draw with drawArrays, drawElements, drawRangeElements or their instanced variants generates an INVALID_OPERATION error if any active uniform block in the program used for the draw command is not backed by a sufficiently large buffer object.

In OpenGL, insufficient uniform block backing is allowed to cause GL interruption or termination. See OpenGL ES 3.0 specification section 2.12.6 ( OpenGL ES 3.0.6 §2.12.6), under "Uniform Buffer Object Bindings."
5.8 Default Framebuffer
WebGL always has a default framebuffer. The FRAMEBUFFER_UNDEFINED enumerant is removed from the WebGL 2.0 API.

5.9 String Length Queries
In the WebGL 2.0 API, the enumerants ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH, TRANSFORM_FEEDBACK_VARYING_MAX_LENGTH, UNIFORM_BLOCK_NAME_LENGTH, and UNIFORM_NAME_LENGTH are removed in addition to similar enumerants removed in the WebGL 1.0 API.

5.10 Invalid Clears
In the WebGL 2.0 API, trying to perform a clear when there is a mismatch between the type of the specified clear value and the type of a buffer that is being cleared generates an INVALID_OPERATION error instead of producing undefined results.

5.11 Invalid Texture Offsets
A GLSL shader which attempts to use a texture offset value outside the range specified by implementation-defined parameters MIN_PROGRAM_TEXEL_OFFSET and MAX_PROGRAM_TEXEL_OFFSET in texture lookup function arguments must fail compilation in the WebGL 2.0 API.

Using an offset outside the valid range returns undefined results, so it can not be allowed. The offset must be a constant expression according to the GLSL ES spec, so checking the value against the correct range can be done at compile time.
5.12 Texel Fetches
Texel fetches that have undefined results in the OpenGL ES 3.0 API must return zero, or a texture source color of (0, 0, 0, 1) in the case of a texel fetch from an incomplete texture in the WebGL 2.0 API.

Behavior of out-of-range texel fetches needs to be testable in order to guarantee security.
5.13 GLSL ES 1.00 Fragment Shader Output
A fragment shader written in The OpenGL ES Shading Language, Version 1.00, that statically assigns a value to gl_FragData[n] where n does not equal constant value 0 must fail to compile in the WebGL 2.0 API. This is to achieve consistency with The OpenGL ES 3.0 specification section 4.2.1 (OpenGL ES 3.0.6 §4.2.1) and The OpenGL ES Shading Language 3.00.6 specification section 1.5 (GLSL ES 3.00.6 §1.5). As in the OpenGL ES 3.0 API, multiple fragment shader outputs are only supported for GLSL ES 3.00 shaders in the WebGL 2.0 API.

5.14 No MapBufferRange
The MapBufferRange, FlushMappedBufferRange, and UnmapBuffer entry points are removed from the WebGL 2.0 API. The following enum values are also removed: BUFFER_ACCESS_FLAGS, BUFFER_MAP_LENGTH, BUFFER_MAP_OFFSET, MAP_READ_BIT, MAP_WRITE_BIT, MAP_INVALIDATE_RANGE_BIT, MAP_INVALIDATE_BUFFER_BIT, MAP_FLUSH_EXPLICIT_BIT, and MAP_UNSYNCHRONIZED_BIT.

Instead of using MapBufferRange, buffer data may be read by using the getBufferSubData entry point.
5.15 TIMEOUT_IGNORED
In the WebGL 2.0 API TIMEOUT_IGNORED is defined as a GLint64 with the value -1 instead of a GLuint64 with the value 0xFFFFFFFFFFFFFFFF. This is because Javascript cannot accurately represent an integer that large. For the same reason waitSync takes GLint64 values instead of GLuint64 for timeout.

5.16 clientWaitSync
In the WebGL 2.0 API, WebGL implementations must enforce a short maximum timeout on calls to clientWaitSync in order to avoid blocking execution of the main thread for excessive periods of time. This timeout may be queried by calling getParameter with the argument MAX_CLIENT_WAIT_TIMEOUT_WEBGL.

The implementation-defined maximum timeout is not specified. It is acceptable for an implementation to enforce a zero maximum timeout.
5.17 Vertex Attribute Aliasing
WebGL 2.0 API implementations must strictly follow GLSL ES 3.00.6 section 12.46, which specifies that any vertex attribute aliasing is disallowed. As stated in [GLES30] p57, GLSL ES 1.00 shaders may still alias, as allowed by the WebGL 1.0 spec section Attribute aliasing.

5.18 PRIMITIVE_RESTART_FIXED_INDEX is always enabled
The PRIMITIVE_RESTART_FIXED_INDEX context state, controlled with Enable/Disable in OpenGL ES 3.0, is not supported in WebGL 2.0. Instead, WebGL 2.0 behaves as though this state were always enabled. This is a compatibility difference compared to WebGL 1.0.

When drawElements, drawElementsInstanced, or drawRangeElements processes an index, if the index's value is the maximum for the data type (255 for UNSIGNED_BYTE indices, 65535 for UNSIGNED_SHORT, or 4294967295 for UNSIGNED_INT), then the vertex is not processed normally. Instead, it is as if the drawing command ended with the immediately preceding vertex, and another drawing command is immediately started with the same parameters, but only transferring the immediately following index through the end of the originally specified indices.

This compatibility difference was introduced in order to avoid performance pitfalls in Direct3D based WebGL implementations. Applications and content creation tools can be adjusted to avoid using the maximum vertex index if the primitive restart behavior is not desired.
5.19 No texture swizzles
OpenGL ES 3.0 introduces new state on texture objects allowing a four-channel swizzle operation to be specified against the texture. The swizzle is applied to every texture lookup performed within any shader referencing that texture. These texture swizzles are not supported in WebGL 2.0. TEXTURE_SWIZZLE_* enum values are removed from the WebGL 2.0 API.

Texture swizzles can not be implemented in a performant manner on Direct3D based WebGL implementations. Applications relying on this functionality would run significantly more slowly on those implementations. Applications are still able to swizzle results of texture fetches in shaders and swizzle texture data before uploading without this interface.
5.20 Queries should fail on a program that failed to link
OpenGL ES 3.0 allows applications to enumerate and query properties of active variables and interface blocks of a specified program even if that program failed to link (OpenGL ES 3.0.6 §2.12.3). In WebGL, these commands will always generate an INVALID_OPERATION error on a program that failed to link, and no information is returned.

The returned information in OpenGL ES 3.0 is implementation dependent and may be incomplete. The error condition is added to ensure consistent behavior across all platforms.
5.21 Color values from a fragment shader must match the color buffer format
Color values written by a fragment shader may be floating-point, signed integer, or unsigned integer. If the values written by the fragment shader do not match the format(s) of the corresponding color buffer(s), the result is undefined in OpenGL ES 3.0 (OpenGL ES 3.0.6 §3.9.2.3). In WebGL, generates an INVALID_OPERATION error in the corresponding draw call, including drawArrays, drawElements, drawArraysInstanced , drawElementsInstanced , and drawRangeElements.

If the color buffer has a normalized fixed-point format, floating-point color values are converted to match the format; generates no error in such situation.

5.22 A sampler type must match the internal texture format
Texture lookup functions return values as floating point, unsigned integer or signed integer, depending on the sampler type passed to the lookup function. If the wrong sampler type is used for texture access, i.e., the sampler type does not match the texture internal format, the returned values are undefined in OpenGL ES Shading Language 3.00.6 (OpenGL ES Shading Language 3.00.6 §8.8). In WebGL, generates an INVALID_OPERATION error in the corresponding draw call, including drawArrays, drawElements, drawArraysInstanced, drawElementsInstanced , and drawRangeElements.

If the sampler type is floating point and the internal texture format is normalized integer, it is considered as a match and the returned values are converted to floating point in the range [0, 1].

5.23 Queries' results must not be made available in the current frame
In OpenGL ES 3.0, if the appropriate primitives (e.g. glFinish() or another synchronous API) are called, a query's result may be made available in the same frame it was issued. In WebGL, in order to improve application portability, a query's result must never be made available to the application in the same frame the query was issued. See the specification of getQueryParameter for discussion and rationale.

5.24 GLSL ES 3.00 #extension directive location
The WebGL 1.0 specification section GLSL ES #extension directive location only applies to OpenGL ES Shading Language 1.00 shaders. It does not apply to shaders that are written in OpenGL ES Shading Language 3.00. In shaders written in OpenGL ES Shading Language 3.00, #extension directives must occur before non-preprocessor tokens regardless of what is written in the extension specification.

This is done to make WebGL 2.0 to follow the written GLSL ES 3.00 specification more closely. Enforcing the restriction more strictly than native GLES drivers makes the behavior well-defined.
5.25 Only std140 layout supported in uniform blocks
The GLSL ES 3.00 specification supports the shared, packed, and std140 layout qualifiers for uniform blocks, defining how variables are laid out in uniform buffers' storage. Of these, the WebGL 2.0 specification supports only the std140 layout, which is defined in OpenGL ES 3.0.6 §2.12 "Vertex Shaders", subsection "Standard Uniform Block Layout". Shaders attempting to use the shared or packed layout qualifiers will fail either the compilation or linking stages.

The initial state of compilation is as if the following were declared:

layout(std140) uniform;

This restriction is enforced to improve portability by avoiding exposing uniform block layouts that are specific to one vendor's GPUs.
5.26 Disallowed variants of GLSL ES 3.00 operators
In the WebGL 2.0 API, the following shading language constructs are not allowed and attempting to use them must result in a compile error:

Ternary operator applied to void, arrays, or structs containing arrays
Sequence operator applied to void, arrays, or structs containing arrays
This restriction ensures easy portability across OpenGL ES 3.0 supporting devices.
5.27 checkFramebufferStatus may return FRAMEBUFFER_INCOMPLETE_DIMENSIONS
All attached images much have the same width and height; otherwise, checkFramebufferStatus returns FRAMEBUFFER_INCOMPLETE_DIMENSIONS.

In OpenGL ES 3, attached images for a framebuffer need not to have the same width and height to be framebuffer complete, and FRAMEBUFFER_INCOMPLETE_DIMENSIONS is not one of the valid return values for checkFramebufferStatus. However, in Direct3D 11, on top of which OpenGL ES 3 behavior is emulated in Windows, all render targets must have the same size in all dimensions (see msdn manual page). Emulation of the ES3 semantic on top of DirectX 11 will be inefficient. In order to have consistent WebGL 2.0 behaviors across platforms, it is reasonable to keep the OpenGL ES 2 / WebGL 1.0 restriction for WebGL 2.0 that all attached images must have the same width and height.
5.28 Uniform block matching
In the WebGL 2.0 API, layout qualifiers row_major and column_major are required to match in matched uniform blocks even when they are applied exclusively on non-matrix variables.

This uniform block matching rule is known to be inconsistent across OpenGL ES 3.0 implementations.
5.29 Framebuffer contents after invalidation
In OpenGL ES 3.0, after calling invalidateFramebuffer or invalidateSubFramebuffer, the affected region's contents become effectively undefined. In WebGL 2.0, it is required for the contents to either stay unchanged or become cleared to their default values. (e.g. 0 for colors and stencil, and 1.0 for depth)

It is acceptable for WebGL 2.0 implementations to make invalidateFramebuffer or invalidateSubFramebuffer a no-op.
5.30 No ArrayBufferView matching texture type FLOAT_32_UNSIGNED_INT_24_8_REV
In texImage2D and texImage3D with ArrayBufferView, if type is FLOAT_32_UNSIGNED_INT_24_8_REV and srcData is not null, generates an INVALID_OPERATION.

In texSubImage2D and texSubImage3D with ArrayBufferView, if type is FLOAT_32_UNSIGNED_INT_24_8_REV, generates an INVALID_ENUM.

5.31 VertexAttrib function must match shader attribute type
If any of the following situations are true, attempting to draw with drawArrays, drawElements, drawRangeElements or their instanced variants generates an INVALID_OPERATION error:

vertexAttribPointer, vertexAttrib{1234}f, or vertexAttrib{1234}fv is used and the base type of the shader attribute at slot index is not floating-point (e.g. is signed or unsigned integer);
vertexAttribIPointer is used with type UNSIGNED_BYTE, UNSIGNED_SHORT or UNSIGNED_INT, or vertexAttribI4ui or vertexAttribI4uiv is used, and the base type of the shader attribute at slot index is not unsigned integer (e.g. is floating-point or signed integer);
vertexAttribIPointer is used with type BYTE, SHORT or INT, or vertexAttribI4i or vertexAttribI4iv is used, and the base type of the shader attribute at slot index is not signed integer (e.g. is floating-point or unsigned integer).
This undefined behavior is in the OpenGL ES 3.0 specification section 2.8 ( OpenGL ES 3.0.6 §2.8).
5.32 Transform feedback primitive capture
If any output variable is specified to be streamed to a buffer object but not actually written by a vertex shader, the value is set to 0.

This undefined behavior is in the OpenGL ES 3.0 specification section 2.15.2 ( OpenGL ES 3.0.6 §2.15.2).
5.33 gl_FragDepth
If a fragment shader statically assigns a value to gl_FragDepth, for any fragment where statements assigning a value to gl_FragDepth are not executed, the value 0 is used.

This undefined behavior is in the OpenGL ES 3.0 specification section 3.9.2 ( OpenGL ES 3.0.6 §3.9.2).
5.34 Framebuffer color attachments
If an image is attached to more than one color attachment point in a framebuffer, checkFramebufferStatus returns FRAMEBUFFER_UNSUPPORTED. An image can be an individual mip level, an array slice (from either 2D array or cube map textures), or a 3D texture slice.

This is a limitation of Direct3D 11, on top of which OpenGL ES 3 behavior is emulated on Windows. (see msdn manual page). In order to have consistent WebGL 2.0 behavior across platforms, it is reasonable to add this limitation in WebGL 2.0 for all platforms.
5.35 Pixel store parameters for uploads from TexImageSource
UNPACK_ALIGNMENT and UNPACK_ROW_LENGTH are ignored. UNPACK_ALIGNMENT is specified in bytes, and is implicit and implementation-dependent for TexImageSource objects. UNPACK_ROW_LENGTH is currently unused.

Subrect selection is possible using UNPACK_ params. UNPACK_SKIP_PIXELS and UNPACK_SKIP_ROWS determine the origin of the subrect, with the width and height arguments determining the size of the subrect.

For 3D textures, the width, height, and depth arguments specify a rectangular region of a texel array to unpack to. UNPACK_SKIP_IMAGES and UNPACK_IMAGE_HEIGHT allow selection of multiple slices from the 2D source. UNPACK_IMAGE_HEIGHT determines the stride, in rows, between two slices. For example, a TexImageSource 30 pixels tall may have the top 10 and bottom 10 rows uploaded into two slices of a 3D texture by uploading with height equal to 10, UNPACK_IMAGE_HEIGHT set to 20, and depth equal to 2. If UNPACK_IMAGE_HEIGHT is 0, the stride, in rows, between two slices defaults to height.

For an HTMLImageElement 20 pixels wide, passing width = 10 for texture upload will cause only the left half of the image to be selected, thus uploaded. The resulting texture will have a width of 10. If, additionally in this example, UNPACK_SKIP_PIXELS is set to 10, only the right half of the image is selected for unpack.

Also, UNPACK_SKIP_IMAGES applies only to 3D entrypoints, not to 2D entrypoints.


Looking at another example, the above 32x48 image of six colors, each of size 16x16, is uploaded to a 3D texture of width = 2, height = 1, and depth = 3.

If UNPACK_SKIP_PIXELS is 0, UNPACK_SKIP_ROWS is 0, and UNPACK_IMAGE_HEIGHT is 0, the entire texel array is set to red.
If UNPACK_SKIP_PIXELS is 16, UNPACK_SKIP_ROWS is 16, and UNPACK_IMAGE_HEIGHT is 0, the entire texel array is set to yellow.
If UNPACK_SKIP_PIXELS is 0, UNPACK_SKIP_ROWS is 0, and UNPACK_IMAGE_HEIGHT is 16, the first slice of the texel array is red, the second slice is blue, and the third slice is purple.
If UNPACK_SKIP_PIXELS is 16, UNPACK_SKIP_ROWS is 0, and UNPACK_IMAGE_HEIGHT is 16, the first slice of the texel array is green, the second slice is yellow, and the third slice is pink.
5.36 Pixel store parameter constraints
Define:

DataStoreWidth := ROW_LENGTH ? ROW_LENGTH : width
DataStoreHeight := IMAGE_HEIGHT ? IMAGE_HEIGHT : height
If PACK_SKIP_PIXELS + width > DataStoreWidth, readPixels generates an INVALID_OPERATION error.

If UNPACK_SKIP_PIXELS + width > DataStoreWidth, texImage2D and texSubImage2D generate an INVALID_OPERATION error. This does not apply to texImage2D if no PIXEL_UNPACK_BUFFER is bound and srcData is null.

If UNPACK_SKIP_PIXELS + width > DataStoreWidth or UNPACK_SKIP_ROWS + height > DataStoreHeight, texImage3D and texSubImage3D generate an INVALID_OPERATION error. This does not apply to texImage3D if no PIXEL_UNPACK_BUFFER is bound and srcData is null.

These constraints normalize the use of these parameters to specify a sub region where pixels are stored. For example, in readPixels cases, if we want to store pixels to a sub area of the entire data store, we can use PACK_ROW_LENGTH to specify the data store width, and PACK_SKIP_PIXELS, PACK_SKIP_ROWS, width and height to specify the subarea's xoffset, yoffset, width, and height.

These contraints also forbid row and image overlap, where the situations are not tested in the OpenGL ES 3.0 DEQP test suites, and many drivers behave incorrectly. They also forbid skipping random pixels or rows, but that can be achieved by adjusting the data store offset accordingly.

Further, for uploads from TexImageSource, implied UNPACK_ROW_LENGTH and UNPACK_ALIGNMENT are not strictly defined. These restrictions ensure consistent and efficient behavior regardless of implied UNPACK_ params.

If UNPACK_FLIP_Y_WEBGL or UNPACK_PREMULTIPLY_ALPHA_WEBGL is set to true, texImage2D and texSubImage2D generate an INVALID_OPERATION error if they upload data from a PIXEL_UNPACK_BUFFER.

If UNPACK_FLIP_Y_WEBGL or UNPACK_PREMULTIPLY_ALPHA_WEBGL is set to true, texImage3D and texSubImage3D generate an INVALID_OPERATION error if they upload data from a PIXEL_UNPACK_BUFFER or a non-null client side ArrayBufferView.

5.37 No ETC2 and EAC compressed texture formats
OpenGL ES 3.0 requires support for the following ETC2 and EAC compressed texture formats: COMPRESSED_R11_EAC, COMPRESSED_SIGNED_R11_EAC, COMPRESSED_RG11_EAC, COMPRESSED_SIGNED_RG11_EAC, COMPRESSED_RGB8_ETC2, COMPRESSED_SRGB8_ETC2, COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2, COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2, COMPRESSED_RGBA8_ETC2_EAC, and COMPRESSED_SRGB8_ALPHA8_ETC2_EAC.

These texture formats are not supported by default in WebGL 2.0.

These formats are not natively supported by most desktop GPU hardware. As such, supporting these formats requires software decompression in either the WebGL implementation or the underlying driver. This results in a drastic increase in video memory usage, causing performance losses which are invisible to the WebGL application.

On hardware which supports the ETC2 and EAC compressed texture formats natively (e.g. mobile OpenGL ES 3.0+ hardware), they may be exposed via the extension WEBGL_compressed_texture_etc.

5.38 The value of UNIFORM_BUFFER_OFFSET_ALIGNMENT must be divisible by 4
The value of UNIFORM_BUFFER_OFFSET_ALIGNMENT, as given in basic machine units, must be divisible by 4.

If the value of UNIFORM_BUFFER_OFFSET_ALIGNMENT was not divisible by 4, it would make it impractical to upload ArrayBuffers to uniform buffers which are bound with BindBufferRange.
5.39 Sync objects' results must not be made available in the current frame
In OpenGL ES 3.0, if the appropriate primitives (e.g. glFinish() or another synchronous API) are called, a sync object may be signaled in the same frame it was issued. In WebGL, in order to improve application portability, a sync object must never transition to the signaled state in the same frame the sync was issued. See the specification of getSyncParameter and clientWaitSync for discussion and rationale.

5.40 blitFramebuffer rectangle width/height limitations
blitFramebuffer() src* and dst* parameters must be set so that the resulting width and height of the source and destination rectangles are less than or equal to the maximum value that can be stored in a GLint. In case computing any of the width or height values as a GLint results in integer overflow, blitFramebuffer() generates an INVALID_VALUE error.

Using larger than max 32-bit int sized rectangles for blitFramebuffer triggers issues on most desktop OpenGL drivers, and there is no general workaround for cases where blitFramebuffer is used to scale the framebuffer.
5.41 GenerateMipmap requires positive image dimensions
generateMipmap requires that TEXTURE_BASE_LEVEL's dimensions are all positive.

GLES 3.0.6 technically allows calling GenerateMipmap on 0x0 images, though cubemaps must be cube-complete, thus having positive dimensions.
This change simplifies implementations by allowing a concept of base-level-completeness for textures, as otherwise required for non-mipmap-sampled validation. Since GenerateMipmap has no effect on a 0x0 texture, and is illegal in WebGL 1 (0 is not a power of two), this is an easy restriction to add. Further, GenerateMipmap would otherwise be the only entrypoint that has to operate on undefined texture images, skipping an otherwise-common validation.
5.42 deleteQuery implicitly calls endQuery if the query is active
In GLES, DeleteQueries does not implicitly end queries, even if they are active.

This deviation was not originally specified, but was implicitly standardized across browsers by conformance tests. Some implementations found this behavior simpler to implement. Reverting this behavior to match the GLES specs could break content, and at this point it's better to spec what we implemented.
5.43 Required compressed texture formats
Implementations must support at least one suite of compressed texture formats.


Implementations must support:

WEBGL_compressed_texture_etc AND/OR
(
WEBGL_compressed_texture_s3tc AND
WEBGL_compressed_texture_s3tc_srgb AND
EXT_texture_compression_rgtc
)
To best support our ecosystem, we require implementations to support either ETC2/EAC formats (universal on GLES3 or similar drivers, like many phones) or S3TC formats (universal all other drivers). This guarantees to authors that they can always use compressed textures (including srgb variants) on all devices while maintaining support for as few as two different formats.

There are roughly equivalent formats in each suite for the following uses:

Usage	S3TC/RGTC option (desktop)	ETC2/EAC option (mobile)
R11 unsigned	COMPRESSED_RED_RGTC1_EXT	COMPRESSED_R11_EAC
R11 signed	COMPRESSED_SIGNED_RED_RGTC1_EXT	COMPRESSED_SIGNED_R11_EAC
RG11 unsigned	COMPRESSED_RED_GREEN_RGTC2_EXT	COMPRESSED_RG11_EAC
RG11 signed	COMPRESSED_SIGNED_RED_GREEN_RGTC2_EXT	COMPRESSED_SIGNED_RG11_EAC
RGB8 unsigned	COMPRESSED_RGB_S3TC_DXT1_EXT	COMPRESSED_RGB8_ETC2
RGB8 sRGB	COMPRESSED_SRGB_S3TC_DXT1_EXT	COMPRESSED_SRGB8_ETC2
RGB8 punchthrough unsigned	COMPRESSED_RGBA_S3TC_DXT1_EXT	COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2
RGB8 punchthrough sRGB	COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT	COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2
RGBA8 unsigned	COMPRESSED_RGBA_S3TC_DXT5_EXT	COMPRESSED_RGBA8_ETC2_EAC
RGBA8 sRGB	COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT	COMPRESSED_SRGB8_ALPHA8_ETC2_EAC
5.44 E.g. sampler2DShadow does not guarantee e.g. LINEAR filtering
When TEXTURE_COMPARE_MODE is set to COMPARE_REF_TO_TEXTURE, a texture is always complete even if it uses a LINEAR filtering without the required extensions. However, it is implementation-defined whether the LINEAR filtering occurs. (Implementations may do NEAREST sampling instead)

This functionality is underspecified in GLES. It is common but not universal. We choose to be liberal here, though implementations may warn on requests for this functionality.

5.45 Clamped Constant Blend Color
Implementations may clamp constant blend color on store when the underlying platform does the same. Applications may query BLEND_COLOR parameter to check the effective behavior.

5.46 Generic TRANSFORM_FEEDBACK_BUFFER state location
The TRANSFORM_FEEDBACK_BUFFER generic buffer binding point has been moved to context state, rather than per-object state.

A similar change was made in OpenGL ES 3.2.
5.47 Uninitialized Compressed Textures
In OpenGL ES 2.0 the specification omitted declaring whether it should be possible to create a compressed 2D texture with uninitialized data. In WebGL 1.0, this is explicitly disallowed.

In OpenGL ES 3.0 it is explicitly allowed to create a compressed 2D texture with uninitialized data by passing null to glCompressedTexImage2D. In WebGL 2.0, however, this is still explicitly disallowed. Users can use texStorage2D with a compressed internal format to allocate zero-initialized compressed textures; these textures are however immutable, and not completely compatible with textures allocated via compressedTexImage2D.

6 References
[WEBGL10]
WebGL Specification 1.0.4 Editor's Draft, Dean Jackson, Jeff Gilbert.
[CANVAS]
HTML5: The Canvas Element, World Wide Web Consortium (W3C).
[OFFSCREENCANVAS]
HTML Living Standard - The OffscreenCanvas interface, WHATWG.
[CANVASCONTEXTS]
Canvas Context Registry, WHATWG.
[GLES30]
OpenGL® ES Version 3.0.6, J. Leech, B. Lipchak, August 2019.
[GLES30GLSL]
The OpenGL® ES Shading Language Version 3.00.6, R. Simpson, January 2016.
[REGISTRY]
WebGL Extension Registry
[RFC2119]
Key words for use in RFCs to Indicate Requirement Levels, S. Bradner. IETF, March 1997.
[WEBIDL]
Web IDL - Living Standard, E. Chen, T. Gu, B. Zbarsky, C. McCormack, T. Lange