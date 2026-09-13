ratdisconnected
{
	nopicmip
	{
		map gfx/2d/net.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen identity
		alphaFunc GT0
		depthWrite
	}
}
spawnPoint
{
	cull disable
	{
		map gfx/damage/shadow.tga
		rgbGen entity
		blendfunc blend
		tcMod rotate 86
		tcGen environment 
		alphaGen wave sin 0.5 0 0 0 
	}
}
icons/iconr_green
{
	nopicmip
	{
		map icons/iconr_green.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen oneMinusEntity
	}
}

healthIconRed
{
	nopicmip
	{
		map icons/iconh_red.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
armorIconRed
{
	nopicmip
	{
		map icons/iconr_red.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}

greenArmor1
{
        {
                map textures/sfx/specular.tga          
                tcGen environment
                rgbGen identity
	}  
        {
		map models/powerups/armor/newgreen.tga
                blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen identity
	}
       
         
}

models/powerups/armor/energy_gre1
{
	{
		map models/powerups/armor/energy_gre3.tga 
		blendFunc GL_ONE GL_ONE
		tcMod scroll 7.4 1.3
	}
}

playerBrightOutline05
{
	cull disable
	// add small outline to character
	deformVertexes wave 100 sin 0.5 0 0 0
	cull back
	{
		map gfx/misc/playerBrightShell.tga
		rgbGen entity
		blendfunc gl_src_alpha gl_one
		//blendfunc blend
		alphaGen entity

	}
}

playerBrightOutline05Blend
{
	cull disable
	// add small outline to character
	deformVertexes wave 100 sin 0.5 0 0 0
	cull back
	{
		map gfx/misc/playerBrightShell.tga
		rgbGen entity
		blendfunc blend
		alphaGen entity

	}
}


playerBrightOutline10
{
	cull disable
	// add small outline to character
	deformVertexes wave 100 sin 1.0 0 0 0
	cull back
	{
		map gfx/misc/playerBrightShell.tga
		rgbGen entity
		blendfunc gl_src_alpha gl_one
		//blendfunc blend
		alphaGen entity

	}
}

playerBrightOutline08
{
	cull disable
	// add small outline to character
	deformVertexes wave 100 sin 0.8 0 0 0
	cull back
	{
		map gfx/misc/playerBrightShell.tga
		rgbGen entity
		blendfunc gl_src_alpha gl_one
		//blendfunc blend
		alphaGen entity

	}
}

playerBrightOutline08Blend
{
	cull disable
	// add small outline to character
	deformVertexes wave 100 sin 0.8 0 0 0
	cull back
	{
		map gfx/misc/playerBrightShell.tga
		rgbGen entity
		blendfunc blend
		alphaGen entity

	}
}

playerBrightOutlineOp10
{
	cull disable
	// add small outline to character
	deformVertexes wave 100 sin 1.0 0 0 0
	cull back
	{
		map gfx/misc/playerBrightShell.tga
		rgbGen entity
		//blendfunc gl_src_alpha gl_one
		blendfunc blend
		alphaGen entity

	}
}

playerBrightShell
{
	cull disable
	// add small outline to character
	//deformVertexes wave 100 sin 0.5 0 0 0
	deformVertexes wave 100 sin 0.8 0 0 0
	{
		map gfx/misc/playerBrightShell.tga
		rgbGen entity
		blendfunc gl_src_alpha gl_one
		alphaGen entity

	}
}

playerBrightShellBlend
{
	cull disable
	// add small outline to character
	//deformVertexes wave 100 sin 0.5 0 0 0
	deformVertexes wave 100 sin 0.8 0 0 0
	{
		map gfx/misc/playerBrightShell.tga
		rgbGen entity
		blendfunc blend
		alphaGen entity

	}
}

playerBrightShellFlatBlend
{
	//cull disable
	//deformVertexes wave 100 sin 1 0 0 0 
	//// add small outline to character
	//deformVertexes wave 100 sin 0.5 0 0 0
	{
		map gfx/misc/playerBrightShell.tga
		rgbGen entity
		blendfunc blend
		alphaGen entity

	}
}

playerBrightShellFlat
{
	//cull disable
	//deformVertexes wave 100 sin 1 0 0 0 
	//// add small outline to character
	//deformVertexes wave 100 sin 0.5 0 0 0
	{
		map gfx/misc/playerBrightShell.tga
		rgbGen entity
		blendfunc gl_src_alpha gl_one
		//alphaGen wave sin 0.5 0 0 0
		alphaGen entity

	}
}

playerIceShell
{
	cull disable
	nopicmip
	deformVertexes wave 100 sin 4 0 0 0 
	{
		map textures/oafx/iceshell
		blendFunc blend
		//tcMod scale 0.5 0.5
		tcGen environment
		alphaGen entity
	}
}
playerThawingShell
{
	cull disable
	nopicmip
	deformVertexes wave 100 sin 4 0 0 0 
	{
		map textures/oafx/iceshell
		blendFunc blend
		tcMod scroll 2 1
		//tcMod scale 0.5 0.5
		tcGen environment
		alphaGen entity
	}
}


powerups/ratQuad
{
	cull disable
	deformVertexes wave 100 sin 3 0 0 0 
	{
		map textures/oafx/quadmultshell
		blendfunc gl_src_alpha gl_one
		tcMod scroll 2 1
		//alphaGen wave sin 0.6 0 0 0
		tcGen environment
	}
	{
		map gfx/fx/spec/spots.tga
		blendfunc gl_src_alpha gl_one
		//rgbGen const ( 0.266667 0.423529 0.658824 ) // #446ba8
		rgbGen const ( 0.30196 0.76470 1.0 ) // #4dc3ff
		tcMod scroll 2 1
		tcGen environment 
		alphaGen lightingSpecular
		detail
	}
}

powerups/ratQuadGreyAlpha
{
	cull disable
	deformVertexes wave 100 sin 3 0 0 0 
	{
		map textures/oafx/quadshell_grey_bright.tga
		rgbGen entity
		blendfunc gl_src_alpha gl_one
		tcMod scroll 2 1
		tcGen environment
		alphaGen entity
	}
}

powerups/ratQuadGrey
{
	cull disable
	deformVertexes wave 100 sin 3 0 0 0 
	{
		map textures/oafx/quadshell_grey.tga
		rgbGen entity
		blendfunc gl_src_alpha gl_one
		tcMod scroll 2 1
		alphaGen wave sin 0.6 0 0 0
		tcGen environment
	}
}

powerups/ratQuadSpots
{
	cull disable
	deformVertexes wave 100 sin 3 0 0 0 
	{
		map textures/oafx/quadshell_spots.tga
		blendfunc gl_src_alpha gl_one
		tcMod scroll 2 1
		alphaGen wave sin 0.6 0 0 0
		tcGen environment
	}
	{
		map gfx/fx/spec/spots.tga
		blendfunc gl_src_alpha gl_one
		//rgbGen const ( 0.266667 0.423529 0.658824 ) // #446ba8
		rgbGen const ( 0.30196 0.76470 1.0 ) // #4dc3ff
		tcMod scroll 2 1
		tcGen environment 
		alphaGen lightingSpecular
		detail
	}
}


powerups/ratRegen
{
	deformVertexes wave 100 sin 0.75 0 0 0 
	{
		map textures/oafx/regenshell.tga
		blendfunc gl_src_alpha gl_one
		tcMod rotate 75
		tcGen environment
		//alphaGen wave sin 0.5 0 0 0
		alphaGen wave sin 0.85 0 0 0
	}
	{
		map gfx/fx/spec/skin.tga
		blendfunc gl_src_alpha gl_one
		rgbGen const ( 1 0.50196 0.50196 ) // #ff8080
		tcMod scroll 0.5 0.5
		tcGen environment 
		alphaGen lightingSpecular
		detail
	}
}


powerups/ratBattleSuit
{
	deformVertexes wave 100 sin 1.25 0 0 0 
	{
		map textures/oafx/suitshell
		blendfunc gl_src_alpha gl_one
		tcMod rotate 75
		tcGen environment
		alphaGen wave sin 0.6 0 0 0
		//alphaGen wave sin 0.5 0 0 0
	}
	{
		map gfx/fx/spec/skin.tga
		blendfunc gl_src_alpha gl_one
		//rgbGen const ( 0.74902 0.403922 0.176471 ) // #bf672d
		rgbGen const ( 1.0 0.65490 0.25882 ) // #ffa742
		tcMod scroll 0.5 0.5
		tcGen environment 
		alphaGen lightingSpecular
		detail
	}
}

plasmaTrail
{
	nopicmip
        cull disable
        {
                clampmap sprites/plasmaTrail.tga
                //blendfunc gl_src_alpha gl_one
                blendfunc blend
                alphaGen vertex
        }
}

sprites/typing
{
	nopicmip
	{
		map sprites/ratballoon.tga
		blendfunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaFunc GT0
		depthWrite
	}
}

models/powerups/medkit_use
{
	{
		map models/powerups/medkit_use.tga
		blendfunc blend
		//rgbGen identity
		//rgbGen entity
		alphaGen entity
	}
}


gfx/2d/bigchars16
{
        nopicmip
        nomipmaps
        {
                map gfx/2d/bigchars16.tga
                blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
                rgbgen vertex
        }
}

gfx/2d/bigchars32
{
        nopicmip
        nomipmaps
        {
                map gfx/2d/bigchars32.tga
                blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
                rgbgen vertex
        }
}

gfx/2d/bigchars64
{
        nopicmip
        nomipmaps
        {
                map gfx/2d/bigchars64.tga
                blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
                rgbgen vertex
        }
}

gfx/2d/bigcharsHiRes
{
        nopicmip
        nomipmaps
        {
                map gfx/2d/bigcharsHiRes.tga
                blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
                rgbgen vertex
        }
}
