models/flags/b_flag
{
	nopicmip
	cull disable
	deformVertexes wave 80 sin 2 8 0 3 
	//deformVertexes wave 20 square 0 2 0.5 0.2 
	{
		clampmap models/flags/b_flag.tga
		blendFunc GL_ONE GL_ZERO
		alphaFunc GE128
		depthWrite
		rgbGen identity

		// old:
		////blendFunc blend
		//rgbGen identity
		//alphaFunc GT0
		//depthWrite
	}
}

models/flags/r_flag
{
	nopicmip
	cull disable
	deformVertexes wave 80 sin 2 8 0 3 
	//deformVertexes wave 20 square 0 2 0.5 0.2 
	{
		clampmap models/flags/r_flag.tga
		blendFunc GL_ONE GL_ZERO
		alphaFunc GE128
		depthWrite
		rgbGen identity

		// old:
		////blendFunc blend
		//rgbGen identity
		//alphaFunc GT0
		//depthWrite
	}
}
