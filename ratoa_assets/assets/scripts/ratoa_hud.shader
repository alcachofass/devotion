rat_bardot
{
	nopicmip
	{
		clampmap gfx/2d/hud/statusbar/bardot1.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}
rat_bardot_additiveglow
{
// THIS ONE IS GREAT
	nopicmip
	nomipmaps
	{
		clampmap gfx/2d/hud/statusbar/bardot_evenmoreglow.tga
		//blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		blendFunc add
		rgbGen vertex
	}
}
rat_bardot_transparentglow
{
	nopicmip
	nomipmaps
	{
		clampmap gfx/2d/hud/statusbar/bardot_evenmoreglow_transparent.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		//blendFunc add
		rgbGen vertex
	}
}
rat_bardot_outline
{
	nopicmip
	nomipmaps
	{
		clampmap gfx/2d/hud/statusbar/bardot_outline.tga
		//blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		blendFunc add
		rgbGen vertex
	}
}
rat_bardot_glow
{
	nopicmip
	nomipmaps
	{
		clampmap gfx/2d/hud/statusbar/bardot_glow.tga
		//blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		blendFunc add
		rgbGen vertex
	}
}
rat_bardot_moreglow
{
	nopicmip
	nomipmaps
	{
		clampmap gfx/2d/hud/statusbar/bardot_moreglow.tga
		//blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		blendFunc add
		rgbGen vertex
	}
}
rat_bardot_evenmoreglow
{
// THIS ONE IS GREAT
	nopicmip
	nomipmaps
	{
		clampmap gfx/2d/hud/statusbar/bardot_evenmoreglow.tga
		//blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		blendFunc add
		rgbGen vertex
	}
}
rat_bardot_evenmoreglow_transparent
{
	nopicmip
	nomipmaps
	{
		clampmap gfx/2d/hud/statusbar/bardot_evenmoreglow_transparent.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		//blendFunc add
		rgbGen vertex
	}
}
weapselect
{
	nopicmip
	{
		clampmap gfx/2d/hud/weapselect.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}
weapselectSquare
{
	nopicmip
	{
		clampmap gfx/2d/hud/weapselectSquare.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}
weapselectRect2
{
	nopicmip
	{
		clampmap gfx/2d/hud/weapselectRect2.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		//blendFunc add
		rgbGen vertex
	}
}
weapselectTechBorder
{
	nopicmip
	{
		clampmap gfx/2d/hud/weapselect_tech_border.tga
		blendFunc add
		rgbGen vertex
	}
}
weapselectTech
{
	nopicmip
	{
		clampmap gfx/2d/hud/weapselect_tech.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		//blendFunc add
		rgbGen vertex
	}
}
weapselectTechCircle
{
	nopicmip
	{
		clampmap gfx/2d/hud/weapselect_techcircle.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}
noammoCircle
{
	nopicmip
	{
		clampmap gfx/2d/hud/weapselect_techcircle_noammo.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}
weapselectTechCircleGlow
{
	nopicmip
	{
		clampmap gfx/2d/hud/weapselect_techcircle_glow.tga
		//blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		blendFunc add
		rgbGen vertex
	}
}
damageIndicatorBottom
{
	nopicmip
	{
		clampmap gfx/2d/hud/damage_indicator_bottom.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}
damageIndicatorTop
{
	nopicmip
	{
		clampmap gfx/2d/hud/damage_indicator_top.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}
damageIndicatorRight
{
	nopicmip
	{
		clampmap gfx/2d/hud/damage_indicator_right.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}
damageIndicatorLeft
{
	nopicmip
	{
		clampmap gfx/2d/hud/damage_indicator_left.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}
damageIndicator2
{
	sort nearest
	nopicmip
	{
		clampmap gfx/2d/hud/damage_indicator2.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
		alphaGen vertex
	}
}
bottomFPSDecorColor
{
	nopicmip
	{
		clampmap gfx/2d/hud/lago_fps_color.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen identity
	}
}
bottomFPSDecorDecor
{
	nopicmip
	{
		clampmap gfx/2d/hud/lago_fps_decor.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}
powerupFrame
{
	nopicmip
	{
		clampmap gfx/2d/hud/powerup_frame.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen identity
	}
}
//teamIndicator
//{
//	nopicmip
//	{
//		clampmap gfx/2d/hud/team_indicator.tga
//		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
//		rgbGen vertex
//	}
//}

rsb4_health_bg
{
	nopicmip
	{
		clampmap gfx/2d/hud/rsb4_health_bg.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}
rsb4_health_bg_border
{
	nopicmip
	{
		clampmap gfx/2d/hud/rsb4_health_bg_border.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}
rsb4_armor_bg
{
	nopicmip
	{
		clampmap gfx/2d/hud/rsb4_armor_bg.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}
rsb4_armor_bg_border
{
	nopicmip
	{
		clampmap gfx/2d/hud/rsb4_armor_bg_border.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}
