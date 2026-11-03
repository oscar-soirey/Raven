/**
 * Raven - widget/anim.c
 * Springs, the transitions that play when a widget appears, and the
 * settings of the animations.
 *
 * A spring is a damped oscillator pulled toward a target:
 *
 *     x'' = -w^2 (x - target) - 2 z w x'
 *
 * `w` sets how fast it answers (its period without friction is the
 * "response", in seconds) and `z` how much it bounces: at 1 it lands without
 * overshoot, below 1 it runs past the target and swings back. The motion is
 * computed from the exact solution of the equation, so it does not depend on
 * the frame rate and never explodes after a long frame.
 */

#include "widget.h"

typedef struct rvn_motion_params_t {
	/** Period of the spring without friction, in seconds. */
	float response;
	/** Damping ratio: 1 = no bounce. */
	float damping;
} rvn_motion_params_t;

static rvn_motion_params_t rvn_motion_params(RMotion motion) {
	rvn_motion_params_t params;
	switch (motion) {
		case R_MOTION_SMOOTH: params.response = 0.30f; params.damping = 1.0f; break;
		case R_MOTION_BOUNCY: params.response = 0.38f; params.damping = 0.52f; break;
		case R_MOTION_WOBBLY: params.response = 0.50f; params.damping = 0.32f; break;
		case R_MOTION_SNAPPY:
		default: params.response = 0.16f; params.damping = 0.90f; break;
	}
	return params;
}

void rvn_spring_set(RSpring* spring, float value) {
	spring->value = value;
	spring->velocity = 0.0f;
	spring->started = R_TRUE;
}

void rvn_spring_kick(RSpring* spring, float velocity) {
	spring->velocity += velocity;
	spring->started = R_TRUE;
}

int rvn_spring_moving(const RSpring* spring, float target) {
	return spring->started && (fabsf(spring->value - target) > 0.0015f || fabsf(spring->velocity) > 0.02f);
}

void rvn_paint_keep_animating(RPainter* painter) {
	if (painter->window) rvn_atomic_store(&painter->window->animating, 1);
}

/** Advances the spring by `dt` seconds. */
static void rvn_spring_step(RSpring* spring, float target, RMotion motion, float dt) {
	rvn_motion_params_t params = rvn_motion_params(motion);
	float omega = 6.2831853f / params.response;
	float zeta = params.damping;
	float x = spring->value - target;
	float v = spring->velocity;
	float decay = expf(-zeta * omega * dt);

	if (zeta < 0.999f) {
		float omega_d = omega * sqrtf(1.0f - zeta * zeta);
		float c = cosf(omega_d * dt), s = sinf(omega_d * dt);
		float b = (v + zeta * omega * x) / omega_d;
		spring->value = target + decay * (x * c + b * s);
		spring->velocity = decay * ((b * omega_d - zeta * omega * x) * c - (x * omega_d + zeta * omega * b) * s);
	} else {
		/* critically damped */
		float b = v + omega * x;
		spring->value = target + decay * (x + b * dt);
		spring->velocity = decay * (b - omega * (x + b * dt));
	}
}

float rvn_paint_spring_from(RPainter* painter, RSpring* spring, float target, RMotion motion) {
	spring->started = R_TRUE;
	/* the looks of the platforms do not bounce */
	if (painter->calm && (motion == R_MOTION_BOUNCY || motion == R_MOTION_WOBBLY)) motion = R_MOTION_SNAPPY;
	if (!g_rvn.animations_disabled && rvn_spring_moving(spring, target)) {
		rvn_spring_step(spring, target, motion, painter->dt);
		if (rvn_spring_moving(spring, target)) {
			rvn_paint_keep_animating(painter);
			return spring->value;
		}
	}
	spring->value = target;
	spring->velocity = 0.0f;
	return target;
}

float rvn_paint_spring(RPainter* painter, RSpring* spring, float target, RMotion motion) {
	if (!spring->started) rvn_spring_set(spring, target);
	return rvn_paint_spring_from(painter, spring, target, motion);
}

float Rdraw_spring(RPainter* painter, RSpring* spring, float target, RMotion motion) {
	if (!painter || !spring) return target;
	if (motion < R_MOTION_SNAPPY || motion > R_MOTION_WOBBLY) motion = R_MOTION_SMOOTH;
	return rvn_paint_spring(painter, spring, target, motion);
}

void Rspring_kick(RSpring* spring, float velocity) {
	if (spring) rvn_spring_kick(spring, velocity);
}

void Rspring_set(RSpring* spring, float value) {
	if (spring) rvn_spring_set(spring, value);
}

/* ---------------------------------------------------------------------------
 *  Settings
 * ------------------------------------------------------------------------- */

void Rset_animations_enabled(int enabled) {
	RVN_API_GUARD_VOID();
	g_rvn.animations_disabled = !enabled;
	rvn_repaint_all_windows();
}

int Rget_animations_enabled(void) {
	return !g_rvn.animations_disabled;
}

void Rset_animation_speed(float speed) {
	RVN_API_GUARD_VOID();
	if (!(speed > 0.0f)) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rset_animation_speed: the speed must be positive");
		return;
	}
	g_rvn.animation_speed = speed;
}

float Rget_animation_speed(void) {
	return g_rvn.initialized && g_rvn.animation_speed > 0.0f ? g_rvn.animation_speed : 1.0f;
}

void Rset_animation_time_step(double seconds) {
	RVN_API_GUARD_VOID();
	g_rvn.animation_time_step = seconds > 0.0 ? seconds : 0.0;
}

float rvn_animation_dt(double elapsed) {
	float speed = g_rvn.animation_speed > 0.0f ? g_rvn.animation_speed : 1.0f;
	if (g_rvn.animation_time_step > 0.0) return (float)g_rvn.animation_time_step * speed;
	/* after a pause the animations resume gently instead of jumping */
	return rvn_clampf((float)elapsed, 0.001f, 1.0f / 30.0f) * speed;
}

/* ---------------------------------------------------------------------------
 *  Transitions
 * ------------------------------------------------------------------------- */

static rvn_fx_t* rvn_widget_fx(rvn_widget_t* widget) {
	if (!widget->fx) widget->fx = (rvn_fx_t*)rvn_calloc(sizeof(rvn_fx_t));
	return widget->fx;
}

void rvn_widget_play_transition(rvn_widget_t* widget, RTransition transition, float delay) {
	rvn_fx_t* fx;
	if (transition == R_TRANSITION_NONE) {
		if (widget->fx) widget->fx->playing = R_FALSE;
		return;
	}
	fx = rvn_widget_fx(widget);
	fx->playing_kind = transition;
	fx->delay = rvn_maxf(delay, 0.0f);
	fx->playing = R_TRUE;
	fx->progress.value = 0.0f;
	fx->progress.velocity = 0.0f;
	fx->progress.started = R_TRUE;
	rvn_widget_invalidate_paint(widget);
}

void rvn_widget_appear(rvn_widget_t* widget) {
	if (widget->fx && widget->fx->appear != R_TRANSITION_NONE && widget->fx->appear != 0) {
		rvn_widget_play_transition(widget, widget->fx->appear, widget->fx->appear_delay);
	}
}

int rvn_transition_begin(rvn_widget_t* widget, RPainter* painter, rvn_transition_state_t* state) {
	rvn_fx_t* fx = widget->fx;
	float t, away;
	RMotion motion;

	memset(state, 0, sizeof(*state));
	state->sx = state->sy = 1.0f;
	if (!fx || !fx->playing) return R_TRUE;
	if (g_rvn.animations_disabled) {
		fx->playing = R_FALSE;
		return R_TRUE;
	}
	if (fx->delay > 0.0f) {
		/* waiting for its turn: not shown yet */
		fx->delay -= painter->dt;
		rvn_paint_keep_animating(painter);
		return R_FALSE;
	}

	switch (fx->playing_kind) {
		case R_TRANSITION_FADE: motion = R_MOTION_SMOOTH; break;
		case R_TRANSITION_POP: motion = R_MOTION_BOUNCY; break;
		case R_TRANSITION_ZOOM: motion = R_MOTION_SMOOTH; break;
		case R_TRANSITION_DROP: motion = R_MOTION_WOBBLY; break;
		case R_TRANSITION_SLIDE_LEFT:
		case R_TRANSITION_SLIDE_RIGHT: motion = R_MOTION_SNAPPY; break;
		default: motion = R_MOTION_BOUNCY; break;
	}
	t = rvn_paint_spring_from(painter, &fx->progress, 1.0f, motion);
	if (!rvn_spring_moving(&fx->progress, 1.0f)) {
		fx->playing = R_FALSE;
		return R_TRUE;
	}
	away = 1.0f - t;
	state->active = R_TRUE;
	/* visible well before it lands: the motion is what should be seen */
	state->opacity = rvn_clampf(t * 2.2f, 0.0f, 1.0f);
	switch (fx->playing_kind) {
		case R_TRANSITION_POP:
			state->sx = state->sy = 0.55f + 0.45f * t;
			break;
		case R_TRANSITION_ZOOM:
			state->sx = state->sy = 1.0f + 0.22f * away;
			break;
		case R_TRANSITION_FLY_UP:
			state->dy = 56.0f * away;
			state->sx = state->sy = 1.0f - 0.08f * rvn_clampf(away, 0.0f, 1.0f);
			break;
		case R_TRANSITION_FLY_DOWN:
			state->dy = -56.0f * away;
			state->sx = state->sy = 1.0f - 0.08f * rvn_clampf(away, 0.0f, 1.0f);
			break;
		case R_TRANSITION_FLY_LEFT:
			state->dx = 72.0f * away;
			state->sx = state->sy = 1.0f - 0.08f * rvn_clampf(away, 0.0f, 1.0f);
			break;
		case R_TRANSITION_FLY_RIGHT:
			state->dx = -72.0f * away;
			state->sx = state->sy = 1.0f - 0.08f * rvn_clampf(away, 0.0f, 1.0f);
			break;
		case R_TRANSITION_SLIDE_LEFT:
			state->dx = 28.0f * away;
			break;
		case R_TRANSITION_SLIDE_RIGHT:
			state->dx = -28.0f * away;
			break;
		case R_TRANSITION_DROP:
			/* falls from above and lands with a squash */
			state->dy = -140.0f * away;
			state->sy = 1.0f + 0.25f * rvn_clampf(away, -0.6f, 0.6f);
			state->sx = 1.0f - 0.12f * rvn_clampf(away, -0.6f, 0.6f);
			state->pivot_y = 0.5f;
			break;
		default:
			break;
	}
	return R_TRUE;
}

/* ---------------------------------------------------------------------------
 *  Public
 * ------------------------------------------------------------------------- */

static int rvn_transition_valid(RTransition transition, const char* function) {
	if (transition >= R_TRANSITION_NONE && transition <= R_TRANSITION_SLIDE_RIGHT) return R_TRUE;
	rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "%s: invalid transition", function);
	return R_FALSE;
}

void rvn_widget_set_appear(rvn_widget_t* widget, const RTransition* transition, const float* delay) {
	rvn_fx_t* fx = rvn_widget_fx(widget);
	if (transition) fx->appear = *transition;
	if (delay) fx->appear_delay = rvn_maxf(*delay, 0.0f);
	/* a widget that is not shown yet plays it when it shows up */
	if (fx->appear == R_TRANSITION_NONE || fx->appear == 0) fx->playing = R_FALSE;
	else rvn_widget_play_transition(widget, fx->appear, fx->appear_delay);
}

void Rwidget_set_transition(Rid id, RTransition transition, float delay_seconds) {
	RVN_WIDGET_VOID(widget, id, 0);
	if (!rvn_transition_valid(transition, __func__)) return;
	rvn_widget_set_appear(widget, &transition, &delay_seconds);
}

void Rwidget_play_transition(Rid id, RTransition transition, float delay_seconds) {
	RVN_WIDGET_VOID(widget, id, 0);
	if (!rvn_transition_valid(transition, __func__)) return;
	rvn_widget_play_transition(widget, transition, delay_seconds);
}

int rvn_parse_transition(const char* text, RTransition* transition) {
	static const struct { const char* name; RTransition value; } names[] = {
		{ "None", R_TRANSITION_NONE }, { "Fade", R_TRANSITION_FADE }, { "Pop", R_TRANSITION_POP },
		{ "Zoom", R_TRANSITION_ZOOM }, { "FlyUp", R_TRANSITION_FLY_UP }, { "FlyDown", R_TRANSITION_FLY_DOWN },
		{ "FlyLeft", R_TRANSITION_FLY_LEFT }, { "FlyRight", R_TRANSITION_FLY_RIGHT }, { "Drop", R_TRANSITION_DROP },
		{ "SlideLeft", R_TRANSITION_SLIDE_LEFT }, { "SlideRight", R_TRANSITION_SLIDE_RIGHT }
	};
	size_t i;
	for (i = 0; i < RVN_COUNT_OF(names); i++) {
		if (rvn_stricmp(text, names[i].name) == 0) {
			*transition = names[i].value;
			return R_TRUE;
		}
	}
	return R_FALSE;
}
