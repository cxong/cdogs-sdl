/*
 C-Dogs SDL
 A port of the legendary (and fun) action/arcade cdogs.
 
 Copyright (c) 2013-2014, 2016 Cong Xu
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
 
 Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.
 Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
 */
#pragma once

#include <stdbool.h>

#include <cdogs/c_array.h>
#include <cdogs/grafx.h>
#include <cdogs/pic.h>
#include <cdogs/vector.h>

#include "editor_brush.h"

typedef enum
{
	UITYPE_NONE,
	UITYPE_LABEL,
	UITYPE_TEXTBOX,
	UITYPE_BUTTON,	// click button with picture
	UITYPE_CONTEXT_MENU,	// appear on mouse click
	UITYPE_CUSTOM
} UIType;

#define UI_SELECT_ONLY							1
#define UI_LEAVE_YC								2
#define UI_LEAVE_XC								4
#define UI_SELECT_ONLY_FIRST					8
#define UI_ENABLED_WHEN_PARENT_HIGHLIGHTED_ONLY	16

typedef struct _UIObject
{
	UIType Type;
	int Id;
	int Id2;
	int Flags;
	Vec2i Pos;
	Vec2i Size;
	bool IsVisible;
	// TODO: memory leak with dynamically allocated tooltips
	char *Tooltip;
	struct _UIObject *Parent;
	CArray Children;	// of UIObject *
	struct _UIObject *Highlighted;
	bool DoNotHighlight;
	bool IsBackground;
	union
	{
		// Labels
		const char *(*LabelFunc)(struct _UIObject *, void *);
		// Text boxes
		struct
		{
			char *(*TextLinkFunc)(struct _UIObject *, void *);
			int MaxLen;	// Buffer should hold MaxLen + 1
			char **(*TextSourceFunc)(void *);
			bool IsEditable;
			char *Hint;
		} Textbox;
		// Button
		struct
		{
			Pic *Pic;
			int (*IsDownFunc)(void *);	// whether the button is down
		} Button;
		// Custom
		void (*CustomDrawFunc)(struct _UIObject *, GraphicsDevice *g, Vec2i, void *);
	} u;
	char *Label;
	bool IsDynamicLabel;
	void *Data;
	int IsDynamicData;
	void (*ChangeFunc)(void *, int d);
	// Function that's called when the control is activated with shift key held
	void (*ChangeFuncAlt)(void *, int d);
	// Whether the change func disables parent context menus (default true)
	bool ChangeDisablesContext;
	// Whether calling the change func changes the file (i.e. requires save)
	bool ChangesData;
	// Whether calling the change func requires a reload
	bool ReloadData;
	void (*OnFocusFunc)(struct _UIObject *, void *);
	// Returns whether mission changed
	bool (*OnUnfocusFunc)(void *);
	// Optional function to check whether this UI object should be visible
	void (*CheckVisible)(struct _UIObject *, void *);
} UIObject;

UIObject *UIObjectCreate(UIType type, int id, Vec2i pos, Vec2i size);
void UIObjectSetDynamicLabel(UIObject *o, const char *label);
void UIButtonSetPic(UIObject *o, Pic *pic);
UIObject *UIObjectCopy(const UIObject *o);
void UIObjectDestroy(UIObject *o);
void UIObjectAddChild(UIObject *o, UIObject *c);
void UITabAddChild(UIObject *o, UIObject *c, char *label);
void UIObjectHighlight(UIObject *o, const bool shift);
// Returns whether mission changed
bool UIObjectUnhighlight(UIObject *o, const bool unhighlightParents);
int UIObjectIsHighlighted(UIObject *o);
EditorResult UIObjectChange(UIObject *o, const int d, const bool shift);

// Add and delete chars from the highlighted text box
// Returns whether a change has been made
EditorResult UIObjectAddChar(UIObject *o, char c);
EditorResult UIObjectDelChar(UIObject *o);

void UIObjectDraw(
	UIObject *o, GraphicsDevice *g, Vec2i pos, Vec2i mouse, CArray *drawObjs);

// Get the UIObject that is at pos (e.g. for mouse clicks)
bool UITryGetObject(UIObject *o, Vec2i pos, UIObject **out);

void UITooltipDraw(GraphicsDevice *device, Vec2i pos, const char *s);
