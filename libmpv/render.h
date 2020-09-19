/* Copyright (C) 2018 the mpv developers
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef MPV_CLIENT_API_RENDER_H_
#define MPV_CLIENT_API_RENDER_H_

#include "client.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Overview
 * --------
 *
 * This API can be used to make mpv render using supported graphic APIs (such
 * as OpenGL). It can be used to handle video display.
 *
 * The renderer needs to be created with mpv_render_context_create() before
 * you start playback (or otherwise cause a VO to be created). Then (with most
 * backends) mpv_render_context_render() can be used to explicitly render the
 * current video frame. Use mpv_render_context_set_update_callback() to get
 * notified when there is a new frame to draw.
 * 在开始播放（或创建VO）之前，需要使用mpv_render_context_create（）创建渲染器。
 * 然后（对于大多数后端），可以使用mpv_render_context_render（）显式渲染当前视频帧。
 * 使用mpv_render_context_set_update_callback（）在有新帧要绘制时获得通知。
 *
 * Preferably rendering should be done in a separate thread. If you call
 * normal libmpv API functions on the renderer thread, deadlocks can result
 * (these are made non-fatal with timeouts, but user experience will obviously
 * suffer). See "Threading" section below.
 * *最好在单独的线程中进行渲染。如果在呈现器线程上调用普通的libmpv API函数，
 * 可能会导致死锁（这些死锁在超时时是非致命的，但是用户体验显然会受到影响）。参见下面的“线程”部分。
 *
 * You can output and embed video without this API by setting the mpv "wid"
 * option to a native window handle (see "Embedding the video window" section
 * in the client.h header). In general, using the render API is recommended,
 * because window embedding can cause various issues, especially with GUI
 * toolkits and certain platforms.
 * 通过将mpv“wid”选项设置为本机窗口句柄（请参见client.h头中的“嵌入视频窗口”部分），
 * 您可以在不使用此API的情况下输出和嵌入视频。
 * 一般来说，建议使用renderapi，因为窗口嵌入会导致各种问题，尤其是在GUI工具包和某些平台上。
 *
 * Supported backends
 * ------------------
 *
 * OpenGL: via MPV_RENDER_API_TYPE_OPENGL, see render_gl.h header.
 *
 * Threading
 * ---------
 *
 * You are recommended to do rendering on a separate thread than normal libmpv
 * use.
 * *建议您在一个单独的线程上进行渲染，而不是普通的libmpv使用。
 *
 * The mpv_render_* functions can be called from any thread, under the
 * following conditions:
 *  - only one of the mpv_render_* functions can be called at the same time
 *    (unless they belong to different mpv cores created by mpv_create())
 *     同一时间只能调用一个mpv_render_x函数（除非它们属于由mpv_create（）创建的不同mpv内核）
 *  - never can be called from within the callbacks set with
 *    mpv_set_wakeup_callback() or mpv_render_context_set_update_callback()
 *    永远不能从使用mpv_set_wakeup_callback（）或mpv_render_context_set_update_callback（）设置的回调集中调用
 *  - if the OpenGL backend is used, for all functions the OpenGL context
 *    must be "current" in the calling thread, and it must be the same OpenGL
 *    context as the mpv_render_context was created with. Otherwise, undefined
 *    behavior will occur.
 *    如果使用OpenGL后端，对于所有函数，OpenGL上下文在调用线程中必须是“当前”的，
 *    并且它必须与创建mpv_render_上下文的OpenGL上下文相同。否则，将发生未定义的行为。
 *  - the thread does not call libmpv API functions other than the mpv_render_*
 *    functions, except APIs which are declared as safe (see below). Likewise,
 *    there must be no lock or wait dependency from the render thread to a
 *    thread using other libmpv functions. Basically, the situation that your
 *    render thread waits for a "not safe" libmpv API function to return must
 *    not happen. If you ignore this requirement, deadlocks can happen, which
 *    are made non-fatal with timeouts; then playback quality will be degraded,
 *    除了声明为安全的API之外，线程不调用libmpv API函数，mpv_render_x函数除外。
 *    同样，从呈现线程到使用其他libmpv函数的线程之间必须没有锁或等待依赖关系。
 *    基本上，呈现线程等待“不安全”的libmpv API函数返回的情况不会发生。
 *    如果忽略此要求，则可能会发生死锁，这些死锁会因超时而变得非致命；然后播放质量将降低
 *    and the message
 *          mpv_render_context_render() not being called or stuck.
 *    is logged. If you set MPV_RENDER_PARAM_ADVANCED_CONTROL, you promise that
 *    this won't happen, and must absolutely guarantee it, or a real deadlock
 *    will freeze the mpv core thread forever.
 *    如果您设置了MPV_RENDER_PARAM_ADVANCED_CONTROL，则您保证不会发生这种情况，
 *    并且必须绝对保证，否则真正的死锁将永远冻结MPV核心线程。
 *
 * libmpv functions which are safe to call from a render thread are:
 *  - functions marked with "Safe to be called from mpv render API threads."
 *  - client.h functions which don't have an explicit or implicit mpv_handle
 *    parameter
 *  - mpv_render_* functions; but only for the same mpv_render_context pointer.但仅适用于同一个mpv_render_上下文指针
 *    If the pointer is different, mpv_render_context_free() is not safe. (The
 *    reason is that if MPV_RENDER_PARAM_ADVANCED_CONTROL is set, it may have
 *    to process still queued requests from the core, which it can do only for
 *    the current context, while requests for other contexts would deadlock.
 *    Also, it may have to wait and block for the core to terminate the video
 *    chain to make sure no resources are used after context destruction.)
 *   如果指针不同，则mpv_render_context_free（）不安全。（原因是，如果设置了MPV_RENDER_PARAM_ADVANCED_CONTROL，
 *   则它可能必须处理来自核心的仍在排队的请求，
 *   它只能对当前上下文执行此操作，而对其他上下文的请求则会处理死锁。
 *   还有, 它可能需要等待并阻止核心终止视频链，以确保上下文破坏后没有资源被使用。）
 *  - if the mpv_handle parameter refers to a different mpv core than the one
 *    you're rendering for (very obscure, but allowed)
 *   如果mpv_handle参数引用的mpv内核与要渲染的mpv内核不同（非常模糊，但允许）
 *
 * Context and handle lifecycle
 * ----------------------------
 *
 * Video initialization will fail if the render context was not initialized yet
 * (with mpv_render_context_create()), or it will revert to a VO that creates
 * its own window.
 * 如果渲染上下文尚未初始化（使用mpv_render_context_create（）），视频初始化将失败，或者将还原为创建自己窗口的VO。
 *
 * Currently, there can be only 1 mpv_render_context at a time per mpv core.
 * 目前，每个mpv内核一次只能有1个mpv_render_上下文
 *
 * Calling mpv_render_context_free() while a VO is using the render context is
 * active will disable video.
 * *在VO使用呈现上下文处于活动状态时调用mpv_render_context_free（）将禁用视频。
 *
 * You must free the context with mpv_render_context_free() before the mpv core
 * is destroyed. If this doesn't happen, undefined behavior will result.
 * 在销毁mpv内核之前，必须使用mpv_render_context_free（）释放上下文。如果不发生这种情况，将导致未定义的行为。
 */

/**
 * Opaque context, returned by mpv_render_context_create().不透明上下文，由mpv_render_context_create（）返回。
 */
typedef struct mpv_render_context mpv_render_context;

/**
 * Parameters for mpv_render_param (which is used in a few places such as
 * mpv_render_context_create().
 *
 * Also see mpv_render_param for conventions and how to use it.
 */
typedef enum mpv_render_param_type {
    /**
     * Not a valid value, but also used to terminate a params array. Its value
     * is always guaranteed to be 0 (even if the ABI changes in the future).
     * *不是有效值，但也用于终止params数组。它的值总是保证为0（即使ABI在将来发生变化）。
     */
    MPV_RENDER_PARAM_INVALID = 0,
    /**
     * The render API to use. Valid for mpv_render_context_create().
     *
     * Type: char*
     *
     * Defined APIs:
     *
     *   MPV_RENDER_API_TYPE_OPENGL:
     *      OpenGL desktop 2.1 or later (preferably core profile compatible to
     *      OpenGL 3.2), or OpenGLES 2.0 or later.
     *      Providing MPV_RENDER_PARAM_OPENGL_INIT_PARAMS is required.
     *      It is expected that an OpenGL context is valid and "current" when
     *      calling mpv_render_* functions (unless specified otherwise). It
     *      must be the same context for the same mpv_render_context.
     * OpenGL desktop 2.1或更高版本（最好是与
     *OpenGL 3.2）或OpenGLES 2.0或更高版本。需要提供MPV_RENDER_PARAM_OPENGL_INIT_PARAMS。
     *调用mpv_render_x函数时，OpenGL上下文应该是有效的并且是“当前”的（除非另有指定）。
     * 对于同一mpv_render_context，它必须是相同的上下文。
     */
    MPV_RENDER_PARAM_API_TYPE = 1,
    /**
     * Required parameters for initializing the OpenGL renderer. Valid for
     * mpv_render_context_create().
     * *初始化OpenGL渲染器所需的参数。对mpv_render_context_create（）有效。
     * Type: mpv_opengl_init_params*
     */
    MPV_RENDER_PARAM_OPENGL_INIT_PARAMS = 2,
    /**
     * Describes a GL render target. Valid for mpv_render_context_render().
     * Type: mpv_opengl_fbo*
     */
    MPV_RENDER_PARAM_OPENGL_FBO = 3,
    /**
     * Control flipped rendering. Valid for mpv_render_context_render().
     * Type: int*
     * If the value is set to 0, render normally. Otherwise, render it flipped,
     * which is needed e.g. when rendering to an OpenGL default framebuffer
     * (which has a flipped coordinate system).
     */
    MPV_RENDER_PARAM_FLIP_Y = 4,
    /**
     * Control surface depth. Valid for mpv_render_context_render().控制面深度
     * Type: int*
     * This implies the depth of the surface passed to the render function in
     * bits per channel. If omitted or set to 0, the renderer will assume 8.
     * Typically used to control dithering.
     */
    MPV_RENDER_PARAM_DEPTH = 5,
    /**
     * ICC profile blob. Valid for mpv_render_context_set_parameter().
     * Type: mpv_byte_array*
     * Set an ICC profile for use with the "icc-profile-auto" option. (If the
     * option is not enabled, the ICC data will not be used.)
     */
    MPV_RENDER_PARAM_ICC_PROFILE = 6,
    /**
     * Ambient light in lux. Valid for mpv_render_context_set_parameter().
     * Type: int*
     * This can be used for automatic gamma correction.
     */
    MPV_RENDER_PARAM_AMBIENT_LIGHT = 7,
    /**
     * X11 Display, sometimes used for hwdec. Valid for
     * mpv_render_context_create(). The Display must stay valid for the lifetime
     * of the mpv_render_context.
     * *X11显示器，有时用于hwdec。对mpv_render_context_create（）有效。显示必须在mpv_render_上下文的生存期内保持有效。
     * Type: Display*
     */
    MPV_RENDER_PARAM_X11_DISPLAY = 8,
    /**
     * Wayland display, sometimes used for hwdec. Valid for
     * mpv_render_context_create(). The wl_display must stay valid for the
     * lifetime of the mpv_render_context.
     * Type: struct wl_display*
     */
    MPV_RENDER_PARAM_WL_DISPLAY = 9,
    /**
     * Better control about rendering and enabling some advanced features. 更好地控制渲染和启用一些高级功能。
     * Validfor mpv_render_context_create().
     *
     * This conflates multiple requirements the API user promises to abide if
     * this option is enabled:
     * *如果启用此选项，则API用户承诺遵守的多个要求会相互冲突：
     *
     *  - The API user's render thread, which is calling the mpv_render_*()
     *    functions, never waits for the core. Otherwise deadlocks can happen.
     * API用户的render线程调用mpv_render_*（）函数，从不等待内核。否则会发生死锁。
     *    See "Threading" section.
     *  - The callback set with mpv_render_context_set_update_callback() can now
     *    be called even if there is no new frame. The API user should call the
     *    mpv_render_context_update() function, and interpret the return value
     *    for whether a new frame should be rendered.
     * 使用mpv_render_context_set_update_callback（）的回调集现在可以调用，即使没有新帧。
     * API用户应调用mpv_render_context_update（）函数，并确定是否应呈现新帧。
     *  - Correct functionality is impossible if the update callback is not set,
     *    or not set soon enough after mpv_render_context_create() (the core can
     *    block while waiting for you to call mpv_render_context_update(), and
     *    if the update callback is not correctly set, it will deadlock, or
     *    block for too long).
     * 如果更新回调未设置，或者在mpv_render_context_create（）之后没有很快设置，则不可能有正确的功能
     * （内核在等待您调用mpv_render_context_update（）时可能会阻塞，如果更新回调设置不正确，它将死锁或阻塞太长时间）。
     *
     * In general, setting this option will enable the following features (and
     * possibly more):
     **通常，设置此选项将启用以下功能（可能还有更多功能）：
     *  - "Direct rendering", which means the player decodes directly to a
     *    texture, which saves a copy per video frame ("vd-lavc-dr" option
     *    needs to be enabled, and the rendering backend as well as the
     *    underlying GPU API/driver needs to have support for it).
     * “直接渲染”，这意味着播放器直接解码到一个纹理，这样每个视频帧保存一个副本
     * （“需要启用vd lavc dr”选项，渲染后端以及底层的GPU API/驱动程序需要支持）。
     *  - Rendering screenshots with the GPU API if supported by the backend
     *    (instead of using a suboptimal software fallback via libswscale).
     * 如果后端支持，使用GPU API呈现屏幕截图（而不是通过libswscale使用次优的软件回退）。
     *
     * Type: int*: 0 for disable (default), 1 for enable
     */
    MPV_RENDER_PARAM_ADVANCED_CONTROL = 10,
    /**
     * Return information about the next frame to render. Valid for
     * mpv_render_context_get_info().
     *
     * Type: mpv_render_frame_info*
     *
     * It strictly returns information about the _next_ frame. The implication
     * is that e.g. mpv_render_context_update()'s return value will have
     * MPV_RENDER_UPDATE_FRAME set, and the user is supposed to call
     * mpv_render_context_render(). If there is no next frame, then the
     * return value will have is_valid set to 0.
     * 它严格返回下一帧的信息。这意味着，例如mpv_render_context_update（）的返回值将设置mpv_render_update_FRAME，
     * 用户应该调用mpv_render_context_render（）。如果没有下一帧，则返回值将被设置为0。
     */
    MPV_RENDER_PARAM_NEXT_FRAME_INFO = 11,
    /**
     * Enable or disable video timing. Valid for mpv_render_context_render().
     *
     * Type: int*: 0 for disable, 1 for enable (default)
     *
     * When video is timed to audio, the player attempts to render video a bit
     * ahead, and then do a blocking wait until the target display time is
     * reached. This blocks mpv_render_context_render() for up to the amount
     * specified with the "video-timing-offset" global option. You can set
     * this parameter to 0 to disable this kind of waiting. If you do, it's
     * recommended to use the target time value in mpv_render_frame_info to
     * wait yourself, or to set the "video-timing-offset" to 0 instead.
     * *当视频定时为音频时，播放器会尝试提前一点渲染视频，然后进行阻塞等待，直到达到目标显示时间。
     * 这将阻止mpv_render_context_render（），最大值为“视频定时偏移”全局选项指定的量。您可以将此参数设置为0以禁用此类等待。
     * 如果您这样做，建议您使用mpv_render_frame_info中的目标时间值等待自己，或将“视频计时偏移”设置为0。
     *
     * Disabling this without doing anything in addition will result in A/V sync
     * being slightly off.
     * 禁用此功能而不做任何其他操作将导致A/V同步稍微关闭。
     */
    MPV_RENDER_PARAM_BLOCK_FOR_TARGET_TIME = 12,
    /**
     * Use to skip rendering in mpv_render_context_render().
     *
     * Type: int*: 0 for rendering (default), 1 for skipping
     *
     * If this is set, you don't need to pass a target surface to the render
     * function (and if you do, it's completely ignored). This can still call
     * into the lower level APIs (i.e. if you use OpenGL, the OpenGL context
     * must be set).
     * *如果设置了此选项，则不需要将目标曲面传递给渲染函数（如果设置了，则会完全忽略）。
     * 这仍然可以调用较低级别的api（即，如果使用OpenGL，则必须设置OpenGL上下文）。
     *
     * Be aware that the render API will consider this frame as having been
     * rendered. All other normal rules also apply, for example about whether
     * you have to call mpv_render_context_report_swap(). It also does timing
     * in the same way.
     * *请注意，渲染API会将此帧视为已渲染。所有其他常规规则也适用，
     * 例如是否必须调用mpv_render_context_report_swap（）。它也以同样的方式计时
     */
    MPV_RENDER_PARAM_SKIP_RENDERING = 13,
    /**
     * DRM display, contains drm display handles.
     * Valid for mpv_render_context_create().
     * Type : struct mpv_opengl_drm_params*
     */
    MPV_RENDER_PARAM_DRM_DISPLAY = 14,
    /**
     * DRM osd size, contains osd dimensions.
     * Valid for mpv_render_context_create().
     * Type : struct mpv_opengl_drm_osd_size*
     */
    MPV_RENDER_PARAM_DRM_OSD_SIZE = 15,
} mpv_render_param_type;

/**
 * Used to pass arbitrary parameters to some mpv_render_* functions. The
 * meaning of the data parameter is determined by the type, and each
 * MPV_RENDER_PARAM_* documents what type the value must point to.
 * 用于将任意参数传递给某些mpv_render_x函数。data参数的含义由类型决定，每个MPV_RENDER_PARAM_x*记录值必须指向的类型。
 *
 * Each value documents the required data type as the pointer you cast to
 * void* and set on mpv_render_param.data. For example, if MPV_RENDER_PARAM_FOO
 * documents the type as Something* , then the code should look like this:
 * *每个值记录所需的数据类型，作为投射到void*并在mpv\u render上设置的指针_参数数据.
 * 例如，如果MPV_RENDER_PARAM_FOO将类型记录为Something*，则代码应该如下所示：
 *
 *   Something foo = {...};
 *   mpv_render_param param;
 *   param.type = MPV_RENDER_PARAM_FOO;
 *   param.data = & foo;
 *
 * Normally, the data field points to exactly 1 object. If the type is char*,
 * it points to a 0-terminated string.
 * 通常，数据字段正好指向1个对象。如果类型是char*，则指向以0结尾的字符串。
 *
 * In all cases (unless documented otherwise) the pointers need to remain
 * valid during the call only. Unless otherwise documented, the API functions
 * will not write to the params array or any data pointed to it.
 * *在所有情况下（除非另有说明），指针只需在调用期间保持有效。除非另有说明，否则API函数不会写入params数组或指向它的任何数据。
 *
 * As a convention, parameter arrays are always terminated by type==0. There
 * is no specific order of the parameters required. The order of the 2 fields in
 * this struct is guaranteed (even after ABI changes).
 * *作为惯例，参数数组总是以类型==0终止。所需参数没有特定的顺序。此结构中2个字段的顺序是有保证的（即使在ABI更改之后）。
 */
typedef struct mpv_render_param {
    enum mpv_render_param_type type;
    void *data;
} mpv_render_param;


/**
 * Predefined values for MPV_RENDER_PARAM_API_TYPE.
 */
#define MPV_RENDER_API_TYPE_OPENGL "opengl"

/**
 * Flags used in mpv_render_frame_info.flags. Each value represents a bit in it.
 * *mpv渲染帧中使用的mpv_render_frame_info.flags每个值代表其中的一个位。
 */
typedef enum mpv_render_frame_info_flag {
    /**
     * Set if there is actually a next frame. If unset, there is no next frame
     * yet, and other flags and fields that require a frame to be queued will
     * be unset.
     * *设置是否有下一帧。如果未设置，则还没有下一帧，并且其他需要帧排队的标志和字段也将被取消设置。
     *
     * This is set for _any_ kind of frame, even for redraw requests.
     * 这是为任何类型的帧设置的，即使是重画请求也是如此。
     *
     * Note that when this is unset, it simply means no new frame was
     * decoded/queued yet, not necessarily that the end of the video was
     * reached. A new frame can be queued after some time.
     * *请注意，如果未设置此项，则表示尚未解码/排队新帧，不一定是到达视频的结尾。一段时间后，新帧可以排队。
     *
     * If the return value of mpv_render_context_render() had the
     * MPV_RENDER_UPDATE_FRAME flag set, this flag will usually be set as well,
     * unless the frame is rendered, or discarded by other asynchronous events.
     * *如果mpv_render_context_render（）的返回值设置了mpv_render_UPDATE_FRAME标志，
     * 则通常也会设置此标志，除非该帧被渲染，或被其他异步事件丢弃。
     */
    MPV_RENDER_FRAME_INFO_PRESENT         = 1 << 0,
    /**
     * If set, the frame is not an actual new video frame, but a redraw request.
     * For example if the video is paused, and an option that affects video
     * rendering was changed (or any other reason), an update request can be
     * issued and this flag will be set.
     * *如果设置，则该帧不是实际的新视频帧，而是重画请求。
     *例如，如果暂停视频，并且影响视频呈现的选项已更改（或任何其他原因），则可以发出更新请求并设置此标志。
     *
     * Typically, redraw frames will not be subject to video timing.
     * 通常，重画帧不受视频计时的影响。
     *
     * Implies MPV_RENDER_FRAME_INFO_PRESENT.
     */
    MPV_RENDER_FRAME_INFO_REDRAW          = 1 << 1,
    /**
     * If set, this is supposed to reproduce the previous frame perfectly. This
     * is usually used for certain "video-sync" options ("display-..." modes).
     * Typically the renderer will blit the video from a FBO. Unset otherwise.
     * *如果设置了，这将完美地再现上一帧。这通常用于某些“视频同步”选项（“display-…”模式）。
     *通常，渲染器将从FBO blit视频。否则取消设置。
     *
     * Implies MPV_RENDER_FRAME_INFO_PRESENT.
     */
    MPV_RENDER_FRAME_INFO_REPEAT          = 1 << 2,
    /**
     * If set, the player timing code expects that the user thread blocks on
     * vsync (by either delaying the render call, or by making a call to
     * mpv_render_context_report_swap() at vsync time).
     * *如果设置，播放器计时代码预计用户线程会阻塞vsync
     * （通过延迟render调用，或在vsync时调用mpv_render_context_report_swap（））。
     *
     * Implies MPV_RENDER_FRAME_INFO_PRESENT.
     */
    MPV_RENDER_FRAME_INFO_BLOCK_VSYNC     = 1 << 3,
} mpv_render_frame_info_flag;

/**
 * Information about the next video frame that will be rendered. Can be
 * retrieved with MPV_RENDER_PARAM_NEXT_FRAME_INFO.
 * *有关将渲染的下一个视频帧的信息。可以用MPV_RENDER_PARAM_NEXT_FRAME_INFO检索。
 */
typedef struct mpv_render_frame_info {
    /**
     * A bitset of mpv_render_frame_info_flag values (i.e. multiple flags are
     * combined with bitwise or).
     * *mpv_render_frame_info_标志值的位集（即多个标志与位“或”组合）。
     */
    uint64_t flags;
    /**
     * Absolute time at which the frame is supposed to be displayed. This is in
     * the same unit and base as the time returned by mpv_get_time_us(). For
     * frames that are redrawn, or if vsync locked video timing is used (see
     * "video-sync" option), then this can be 0. The "video-timing-offset"
     * option determines how much "headroom" the render thread gets (but a high
     * enough frame rate can reduce it anyway). mpv_render_context_render() will
     * normally block until the time is elapsed, unless you pass it
     * MPV_RENDER_PARAM_BLOCK_FOR_TARGET_TIME = 0.
     * *帧应该显示的绝对时间。这与mpv_get_time_us（）返回的时间单位和基数相同。
     * 对于重新绘制的帧，或者如果使用了vsync锁定的视频计时（请参见“视频同步”选项），则此值可以为0。
     * “视频定时偏移”选项决定渲染线程获得多少“净空”（但是足够高的帧速率无论如何都可以减少它）。
     * mpv_render_context_render（）通常会阻塞，直到时间过去为止，除非您通过了它
        MPV_RENDER_PARAM_BLOCK_TARGET_TIME=0。
     */
    int64_t target_time;
} mpv_render_frame_info;

/**
 * Initialize the renderer state. Depending on the backend used, this will
 * access the underlying GPU API and initialize its own objects.
 * *初始化渲染器状态。根据所使用的后端，这将访问底层GPU API并初始化它自己的对象。
 *
 * You must free the context with mpv_render_context_free(). Not doing so before
 * the mpv core is destroyed may result in memory leaks or crashes.
 * *必须使用mpv_render_context_free（）释放上下文。在mpv核心被破坏之前不这样做可能会导致内存泄漏或崩溃。
 *
 * Currently, only at most 1 context can exists per mpv core (it represents the
 * main video output).
 * *目前，每个mpv核心最多只能存在1个上下文（它代表主视频输出）。
 *
 * You should pass the following parameters:
 *  - MPV_RENDER_PARAM_API_TYPE to select the underlying backend/GPU API.
 * MPV_RENDER_PARAM_API_TYPE以选择底层后端/GPU API。
 *  - Backend-specific init parameter, like MPV_RENDER_PARAM_OPENGL_INIT_PARAMS.
 * 后端特定的init参数，如MPV_RENDER_PARAM_OPENGL_init_PARAMS。
 *  - Setting MPV_RENDER_PARAM_ADVANCED_CONTROL and following its rules is
 *    strongly recommended.
 * 强烈建议设置MPV_RENDER_PARAM_ADVANCED_CONTROL并遵循其规则。
 *  - If you want to use hwdec, possibly hwdec interop resources.
 * 如果您想使用hwdec，可能使用hwdec互操作资源。
 *
 * @param res set to the context (on success) or NULL (on failure). The value
 *            is never read and always overwritten.
 * @param mpv handle used to get the core (the mpv_render_context won't depend
 *            on this specific handle, only the core referenced by it)
 * @param params an array of parameters, terminated by type==0. It's left
 *               unspecified what happens with unknown parameters. At least
 *               MPV_RENDER_PARAM_API_TYPE is required, and most backends will
 *               require another backend-specific parameter.
 * @return error code, including but not limited to:
 *      MPV_ERROR_UNSUPPORTED: the OpenGL version is not supported
 *                             (or required extensions are missing)
 *      MPV_ERROR_NOT_IMPLEMENTED: an unknown API type was provided, or
 *                                 support for the requested API was not
 *                                 built in the used libmpv binary.
 *      MPV_ERROR_INVALID_PARAMETER: at least one of the provided parameters was
 *                                   not valid.
 */
int mpv_render_context_create(mpv_render_context **res, mpv_handle *mpv,
                              mpv_render_param *params);

/**
 * Attempt to change a single parameter. Not all backends and parameter types
 * support all kinds of changes.
 * 尝试更改单个参数。并非所有后端和参数类型都支持所有类型的更改。
 *
 * @param ctx a valid render context
 * @param param the parameter type and data that should be set
 * @return error code. If a parameter could actually be changed, this returns
 *         success, otherwise an error code depending on the parameter type
 *         and situation.
 */
int mpv_render_context_set_parameter(mpv_render_context *ctx,
                                     mpv_render_param param);

/**
 * Retrieve information from the render context. This is NOT a counterpart to
 * mpv_render_context_set_parameter(), because you generally can't read
 * parameters set with it, and this function is not meant for this purpose.
 * Instead, this is for communicating information from the renderer back to the
 * user. See mpv_render_param_type; entries which support this function
 * explicitly mention it, and for other entries you can assume it will fail.
 * *从呈现上下文中检索信息。这不是mpv_render_context_set_parameter（）的对应项，
 * 因为您通常无法读取使用它设置的参数，而此函数并不用于此目的。
 *相反，这是为了将信息从呈现器传递回用户。请参见mpv_render_param_type；支持此函数的条目明确提到了它，
 * 对于其他条目，您可以假定它将失败。
 *
 * You pass param with param.type set and param.data pointing to a variable
 * of the required data type. The function will then overwrite that variable
 * with the returned value (at least on success).
 * *你把param传给参数类型设置和参数数据指向所需数据类型的变量。然后函数将用返回值覆盖该变量（至少在成功时）。
 *
 * @param ctx a valid render context
 * @param param the parameter type and data that should be retrieved
 * @return error code. If a parameter could actually be retrieved, this returns
 *         success, otherwise an error code depending on the parameter type
 *         and situation. MPV_ERROR_NOT_IMPLEMENTED is used for unknown
 *         param.type, or if retrieving it is not supported.
 */
int mpv_render_context_get_info(mpv_render_context *ctx,
                                mpv_render_param param);

typedef void (*mpv_render_update_fn)(void *cb_ctx);

/**
 * Set the callback that notifies you when a new video frame is available, or
 * if the video display configuration somehow changed and requires a redraw.
 * Similar to mpv_set_wakeup_callback(), you must not call any mpv API from
 * the callback, and all the other listed restrictions apply (such as not
 * exiting the callback by throwing exceptions).
 * *设置一个回调函数，当有新的视频帧可用时，或者如果视频显示配置发生了更改并需要重新绘制，则会通知您。
 *与mpv_set_wakeup_callback（）类似，您不能从回调调用任何mpv API，并且所有列出的限制都适用（例如不通过引发异常退出回调）
 *
 * This can be called from any thread, except from an update callback. In case
 * of the OpenGL backend, no OpenGL state or API is accessed.
 * *这可以从任何线程调用，但更新回调除外。对于OpenGL后端，不访问OpenGL状态或API。
 *
 * Calling this will raise an update callback immediately.
 * *调用此函数将立即引发更新回调。
 *
 * @param callback callback(callback_ctx) is called if the frame should be
 *                 redrawn
 * @param callback_ctx opaque argument to the callback
 */
void mpv_render_context_set_update_callback(mpv_render_context *ctx,
                                            mpv_render_update_fn callback,
                                            void *callback_ctx);

/**
 * The API user is supposed to call this when the update callback was invoked
 * (like all mpv_render_* functions, this has to happen on the render thread,
 * and _not_ from the update callback itself).
 * *API用户应该在调用update回调时调用它（就像所有mpv_render_x函数一样，这必须发生在呈现线程上，而不是来自update回调本身）。
 *
 * This is optional if MPV_RENDER_PARAM_ADVANCED_CONTROL was not set (default).
 * Otherwise, it's a hard requirement that this is called after each update
 * callback. If multiple update callback happened, and the function could not
 * be called sooner, it's OK to call it once after the last callback.
 * *如果未设置MPV_RENDER_PARAM_ADVANCED_CONTROL（默认值），则此选项是可选的。
 *否则，很难在每次更新回调之后调用它。如果发生多个update回调，并且无法更快地调用函数，则可以在最后一次回调之后调用它一次。
 *
 * If an update callback happens during or after this function, the function
 * must be called again at the soonest possible time.
 * *如果在该函数期间或之后发生更新回调，则必须尽快再次调用该函数。
 *
 * If MPV_RENDER_PARAM_ADVANCED_CONTROL was set, this will do additional work
 * such as allocating textures for the video decoder.
 * *如果设置了MPV_RENDER_PARAM_ADVANCED_CONTROL，则这将执行其他工作，例如为视频解码器分配纹理。
 *
 * @return a bitset of mpv_render_update_flag values (i.e. multiple flags are
 *         combined with bitwise or). Typically, this will tell the API user
 *         what should happen next. E.g. if the MPV_RENDER_UPDATE_FRAME flag is
 *         set, mpv_render_context_render() should be called. If flags unknown
 *         to the API user are set, or if the return value is 0, nothing needs
 *         to be done.
 */
uint64_t mpv_render_context_update(mpv_render_context *ctx);

/**
 * Flags returned by mpv_render_context_update(). Each value represents a bit
 * in the function's return value.
 * *mpv_render_context_update（）返回的标志。每个值表示函数返回值中的一个位。
 */
typedef enum mpv_render_update_flag {
    /**
     * A new video frame must be rendered. mpv_render_context_render() must be
     * called.
     */
    MPV_RENDER_UPDATE_FRAME         = 1 << 0,
} mpv_render_context_flag;

/**
 * Render video.
 *
 * Typically renders the video to a target surface provided via mpv_render_param
 * (the details depend on the backend in use). Options like "panscan" are
 * applied to determine which part of the video should be visible and how the
 * video should be scaled. You can change these options at runtime by using the
 * mpv property API.
 * *通常将视频渲染到通过mpv_render_param提供的目标曲面（详细信息取决于使用的后端）。
 * 像“panscan”这样的选项用于确定视频的哪个部分应该可见以及如何缩放视频。您可以使用mpv属性API在运行时更改这些选项。
 *
 * The renderer will reconfigure itself every time the target surface
 * configuration (such as size) is changed.
 * *每次更改目标曲面配置（例如大小）时，渲染器都将重新配置自身。
 *
 * This function implicitly pulls a video frame from the internal queue and
 * renders it. If no new frame is available, the previous frame is redrawn.
 * The update callback set with mpv_render_context_set_update_callback()
 * notifies you when a new frame was added. The details potentially depend on
 * the backends and the provided parameters.
 * *此函数隐式地从内部队列中提取一个视频帧并对其进行渲染。如果没有可用的新帧，则重新绘制上一帧。
 *mpv_render_context_update_callback（）的更新回调集在添加新帧时通知您。细节可能取决于后端和提供的参数。
 *
 * Generally, libmpv will invoke your update callback some time before the video
 * frame should be shown, and then lets this function block until the supposed
 * display time. This will limit your rendering to video FPS. You can prevent
 * this by setting the "video-timing-offset" global option to 0. (This applies
 * only to "audio" video sync mode.)
 * *一般来说，libmpv会在视频帧应该显示之前调用更新回调，然后让这个函数阻塞到预期的显示时间。这将限制渲染为视频FPS。
 * 您可以通过将“视频定时偏移”全局选项设置为0来防止这种情况。（这仅适用于“音频”视频同步模式。）
 *
 * You should pass the following parameters:
 *  - Backend-specific target object, such as MPV_RENDER_PARAM_OPENGL_FBO.
 *  - Possibly transformations, such as MPV_RENDER_PARAM_FLIP_Y.
 *
 * @param ctx a valid render context
 * @param params an array of parameters, terminated by type==0. Which parameters
 *               are required depends on the backend. It's left unspecified what
 *               happens with unknown parameters.
 * @return error code
 */
int mpv_render_context_render(mpv_render_context *ctx, mpv_render_param *params);

/**
 * Tell the renderer that a frame was flipped at the given time. This is
 * optional, but can help the player to achieve better timing.
 * *告诉渲染器在给定时间翻转了一个帧。这是可选的，但可以帮助玩家获得更好的时机。
 *
 * Note that calling this at least once informs libmpv that you will use this
 * function. If you use it inconsistently, expect bad video playback.
 * *请注意，调用此函数至少一次会通知libmpv您将使用此函数。如果你不一致地使用它，可能会导致糟糕的视频播放。
 *
 * If this is called while no video is initialized, it is ignored.
 * *如果在没有初始化视频的情况下调用此函数，则忽略它。
 *
 * @param ctx a valid render context
 */
void mpv_render_context_report_swap(mpv_render_context *ctx);

/**
 * Destroy the mpv renderer state.
 *
 * If video is still active (e.g. a file playing), video will be disabled
 * forcefully.
 * *如果视频仍处于活动状态（如播放文件），视频将被强制禁用。
 *
 * @param ctx a valid render context. After this function returns, this is not
 *            a valid pointer anymore. NULL is also allowed and does nothing.
 */
void mpv_render_context_free(mpv_render_context *ctx);

#ifdef __cplusplus
}
#endif

#endif
