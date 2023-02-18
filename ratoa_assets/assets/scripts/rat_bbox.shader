bbox
{
	//cull disable
        {
                map gfx/misc/playerBrightShell.tga
                //rgbGen entity
                blendfunc gl_src_alpha gl_one
                alphaGen wave sin 0.2 0 0 0

        }
}

bbox_nocull
{
	cull disable
        {
                map gfx/misc/playerBrightShell.tga
                //rgbGen entity
                blendfunc gl_src_alpha gl_one
                //alphaGen entity
                alphaGen wave sin 0.2 0 0 0

        }
}
