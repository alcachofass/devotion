models/players/anarki/ep
{
	nopicmip
	{
		map models/players/anarki/ep.tga
		blendFunc GL_ONE GL_ZERO
		alphaFunc GE128
		rgbGen entity
	}
	{
		map models/players/anarki/ep.tga
		blendFunc GL_ONE GL_ZERO
		alphaFunc LT128
		rgbGen identity
	}
}

models/players/anarki/ep_b
{
	nopicmip
	{
		map textures/effects/tinfx.tga
        tcGen environment
        tcmod rotate 350
        tcmod scroll 3 1
        blendfunc GL_ONE GL_ZERO
		rgbGen identity
	} 
    {
      	map models/players/anarki/ep_b.tga
        blendfunc GL_ONE GL_ZERO
		alphaFunc GE128
		rgbGen identity
    }
}

models/players/anarki/ep_x
{
	nopicmip
    {
      	map models/players/anarki/ep_x.tga
        blendfunc GL_ONE GL_ZERO
		alphaFunc GE128
		rgbGen identity
    }
}

models/players/anarki/ep_h
{
	nopicmip
	{
		map models/players/anarki/ep_h.tga
	}
	{
		map models/players/anarki/ep_h.tga
		blendfunc GL_ONE GL_ZERO
		alphaFunc GE128
		rgbGen entity
	}
}

models/players/biker/ep
{
	nopicmip
	{
		map models/players/biker/ep.tga
	}
	{
		map models/players/biker/ep.tga
		blendfunc GL_ONE GL_ZERO
		alphaFunc GE128
		rgbGen entity
	}
}

models/players/biker/ep_h
{
	nopicmip
	{
		map models/players/biker/ep_h.tga
	}
	{
		map models/players/biker/ep_h.tga
		blendfunc GL_ONE GL_ZERO
		alphaFunc GE128
		rgbGen entity
	}
}

models/players/bitterman/ep
{
	nopicmip
	{
		map models/players/bitterman/ep.tga
	}
	{
		map models/players/bitterman/ep.tga
		blendfunc GL_ONE GL_ZERO
		alphaFunc GE128
		rgbGen entity
	}
}

models/players/bitterman/ep_h
{
	nopicmip
	{
		map models/players/bitterman/ep_h.tga
	}
	{
		map models/players/bitterman/ep_h.tga
		blendfunc GL_ONE GL_ZERO
		alphaFunc GE128
		rgbGen entity
	}
}

models/players/bones/ep
{
	nopicmip
	{
		map models/players/bones/ep.tga
		blendFunc GL_ONE GL_ZERO
		alphaFunc GE128
		rgbGen entity
	}
	{
		map models/players/bones/ep.tga
		blendFunc GL_ONE GL_ZERO
		alphaFunc LT128
		rgbGen identity
	}
}

models/players/crash/ep
{
	nopicmip
	{
		map models/players/crash/ep.tga
	}
	{
		map models/players/crash/ep.tga
		blendfunc GL_ONE GL_ZERO
		alphaFunc GE128
		rgbGen entity
	}
}

models/players/crash/ep_t
{
	nopicmip
	{
		map models/players/crash/ep_t.tga
	}
	{
		map models/players/crash/ep_t.tga
		blendfunc GL_ONE GL_ZERO
		alphaFunc GE128
		rgbGen entity
	}
}

models/players/crash/ep_f
{
	nopicmip
    {
		map textures/sfx/snow.tga
        tcmod scale .5 .5
        tcmod scroll  9 0.3
        rgbGen lightingdiffuse
    }
    {
        map textures/effects/tinfx2b.tga
        tcGen environment
        blendFunc add
        rgbGen lightingdiffuse
    }
}

models/players/doom/ep
{
	nopicmip
	{
		map models/players/doom/ep.tga
	}
	{
		map models/players/doom/ep.tga
		blendfunc GL_ONE GL_ZERO
		alphaFunc GE128
		rgbGen entity
	}
}

models/players/doom/ep_f
{
	nopicmip
	{
		map models/players/doom/doom_f.tga	
        rgbGen lightingdiffuse
	}
    {
		map models/players/doom/doom_fx.tga
		tcGen environment
		blendfunc gl_ONE gl_ONE 		
	}
}

models/players/grunt/ep
{
	nopicmip
	{
		map models/players/grunt/ep.tga
	}
	{
		map models/players/grunt/ep.tga
		blendfunc GL_ONE GL_ZERO
		alphaFunc GE128
		rgbGen entity
	}
}

models/players/grunt/ep_h
{
	nopicmip
	{
		map models/players/grunt/ep_h.tga
	}
	{
		map models/players/grunt/ep_h.tga
		blendfunc GL_ONE GL_ZERO
		alphaFunc GE128
		rgbGen entity
	}
}

models/players/hunter/ep
{
	nopicmip
	{
		map models/players/hunter/ep.tga
	}
	{
		map models/players/hunter/ep.tga
		blendfunc GL_ONE GL_ZERO
		alphaFunc GE128
		rgbGen entity
	}
}

models/players/hunter/ep_f
{
	nopicmip
	deformVertexes wave 100 sin 0 .3 0 .2
    cull disable
	{
		map models/players/hunter/ep_f.tga
        rgbGen lightingdiffuse
        alphaFunc GE128
		depthWrite
	}
	{
		map models/players/hunter/ep2_f.tga
		blendfunc add
		alphaFunc GE128
		rgbGen entity
	}
}

models/players/hunter/ep_h
{
	nopicmip
	{
		map models/players/hunter/ep_h.tga
	}
	{
		map models/players/hunter/ep_h.tga
		blendfunc GL_ONE GL_ZERO
		alphaFunc GE128
		rgbGen entity
	}
}

models/players/keel/ep
{
	nopicmip
	{
		map models/players/keel/ep.tga
		blendFunc GL_ONE GL_ZERO
		alphaFunc GE128
		rgbGen entity
	}
	{
		map models/players/keel/ep.tga
		blendFunc GL_ONE GL_ZERO
		alphaFunc LT128
		rgbGen identity
	}
}

models/players/keel/ep_h
{
	nopicmip
	{
		map models/players/keel/ep_h.tga
		blendFunc GL_ONE GL_ZERO
		alphaFunc GE128
		rgbGen entity
	}
	{
		map models/players/keel/ep_h.tga
		blendFunc GL_ONE GL_ZERO
		alphaFunc LT128
		rgbGen identity
	}
}

models/players/klesk/ep
{
	nopicmip
	{
		map models/players/klesk/ep.tga
	}
	{
		map models/players/klesk/ep.tga
		blendfunc GL_ONE GL_ZERO
		alphaFunc GE128
		rgbGen entity
	}
}

models/players/klesk/ep_h
{
	nopicmip
	{
		map models/players/klesk/ep_h.tga
	}
	{
		map models/players/klesk/ep_h.tga
		blendfunc GL_ONE GL_ZERO
		alphaFunc GE128
		rgbGen entity
	}
}

models/players/lucy/ep
{
	nopicmip
	{
		map models/players/lucy/ep.tga
	}
	{
		map models/players/lucy/ep.tga
		blendfunc GL_ONE GL_ZERO
		alphaFunc GE128
		rgbGen entity
	}
}

models/players/lucy/ep_h
{
	nopicmip
	{
		map models/players/lucy/ep_h.tga
	}
	{
		map models/players/lucy/ep_h.tga
		blendfunc GL_ONE GL_ZERO
		alphaFunc GE128
		rgbGen entity
	}
}

models/players/major/ep
{
	nopicmip
	{
		map models/players/major/ep.tga
	}
	{
		map models/players/major/ep.tga
		blendfunc GL_ONE GL_ZERO
		alphaFunc GE128
		rgbGen entity
	}
}

models/players/major/ep_h
{
	nopicmip
	{
		map models/players/major/ep_h.tga
	}
	{
		map models/players/major/ep_h.tga
		blendfunc GL_ONE GL_ZERO
		alphaFunc GE128
		rgbGen entity
	}
}

models/players/mynx/ep_t
{
	nopicmip
	{
		map models/players/mynx/ep_t.tga
	}
	{
		map models/players/mynx/ep_t.tga
		blendfunc GL_ONE GL_ZERO
		alphaFunc GE128
		rgbGen entity
	}
}

models/players/orbb/ep
{
	nopicmip
	{
		map models/players/orbb/ep.tga
	}
	{
		map models/players/orbb/ep.tga
		blendfunc GL_ONE GL_ZERO
		alphaFunc GE128
		rgbGen entity
	}
}

models/players/orbb/ep_h
{
	nopicmip
	{
		map models/players/orbb/ep_h.tga
	}
	{
		map models/players/orbb/ep_h.tga
		blendfunc GL_ONE GL_ZERO
		alphaFunc GE128
		rgbGen entity
	}
}

models/players/ranger/ep
{
	nopicmip
	{
		map models/players/ranger/ep.tga
	}
	{
		map models/players/ranger/ep.tga
		blendfunc GL_ONE GL_ZERO
		alphaFunc GE128
		rgbGen entity
	}
}

models/players/ranger/ranger_ql_h
{
	nopicmip
	{
		AnimMap 4 models/players/ranger/ranger_h.tga models/players/ranger/ranger_h.tga models/players/ranger/ranger_h.tga models/players/ranger/ranger_h.tga models/players/ranger/ranger_h2.tga models/players/ranger/ranger_h3.tga models/players/ranger/ranger_h2.tga
        rgbGen lightingdiffuse
	}
}

models/players/razor/ep
{
	nopicmip
	{
		map models/players/razor/ep.tga
	}
	{
		map models/players/razor/ep.tga
		blendfunc GL_ONE GL_ZERO
		alphaFunc GE128
		rgbGen entity
	}
}

models/players/razor/ep_h
{
	nopicmip
	{
		map models/players/razor/ep_h.tga
	}
	{
		map models/players/razor/ep_h.tga
		blendfunc GL_ONE GL_ZERO
		alphaFunc GE128
		rgbGen entity
	}
}

models/players/sarge/ep
{
	nopicmip
	{
		map models/players/sarge/ep.tga
	}
	{
		map models/players/sarge/ep.tga
		blendfunc GL_ONE GL_ZERO
		alphaFunc GE128
		rgbGen entity
	}
}

models/players/sarge/cigar
{
	nopicmip
	{
		map models/players/sarge/cigar.tga
        blendfunc GL_ONE GL_ZERO
        rgbGen lightingDiffuse	
    }
	{
		map models/players/sarge/cigar.glow.tga
		blendfunc GL_ONE GL_ONE
		rgbGen wave triangle .5 1 0 .2
	}
}

models/players/slash/ep
{
	nopicmip
	{
		map models/players/slash/ep.tga
	}
	{
		map models/players/slash/ep.tga
		blendfunc GL_ONE GL_ZERO
		alphaFunc GE128
		rgbGen entity
	}
}

models/players/slash/ep_h
{
	nopicmip
	{
		map models/players/slash/ep_h.tga
	}
	{
		map models/players/slash/ep_h.tga
		blendfunc GL_ONE GL_ZERO
		alphaFunc GE128
		rgbGen entity
	}
}

models/players/slash/skate_ql
{
	nopicmip
	sort additive
	cull disable
	{
		clampmap models/players/slash/skate.tga
		blendfunc add
        tcMod stretch sin .9 0.1 0 1.1
        rgbgen identity
	}
}

models/players/sorlag/ep
{
	nopicmip
	{
		map models/players/sorlag/ep.tga
	}
	{
		map models/players/sorlag/ep.tga
		blendfunc GL_ONE GL_ZERO
		alphaFunc GE128
		rgbGen entity
	}
}

models/players/sorlag/ep_t
{
	nopicmip
	{
		map models/players/sorlag/ep_t.tga
	}
	{
		map models/players/sorlag/ep_t.tga
		blendfunc GL_ONE GL_ZERO
		alphaFunc GE128
		rgbGen entity
	}
}

models/players/tankjr/ep
{
	nopicmip
	{
		map models/players/tankjr/ep.tga
		blendFunc GL_ONE GL_ZERO
		alphaFunc GE128
		rgbGen entity
	}
	{
		map models/players/tankjr/ep.tga
		blendFunc GL_ONE GL_ZERO
		alphaFunc LT128
		rgbGen identity
	}
}

models/players/uriel/ep
{
	nopicmip
	{
		map models/players/uriel/ep.tga
	}
	{
		map models/players/uriel/ep.tga
		blendfunc GL_ONE GL_ZERO
		alphaFunc GE128
		rgbGen entity
	}
}

models/players/uriel/ep_h
{
    nopicmip
    {
        map models/players/uriel/uriel_h.tga
        rgbGen lightingDiffuse
        blendfunc GL_ONE GL_ZERO
    } 
    {
        map models/players/uriel/ep_f.tga
        blendfunc add
        tcmod scroll -0.2 1
        rgbGen entity
    }
    {
        map models/players/uriel/uriel_h.tga
        rgbGen lightingDiffuse
        blendfunc blend
    }
}

models/players/uriel/ep_w
{
	nopicmip
	deformVertexes wave 100 sin 0 .5 0 .2
	{
		map models/players/uriel/ep_w.tga
		blendfunc blend
		rgbGen lightingDiffuse
	}
	{
		map models/players/uriel/ep1_w.tga
		blendfunc GL_ONE GL_ZERO
		alphaFunc GE128
		rgbGen entity
	}
}

models/players/visor/ep
{
	nopicmip
	{
		map models/players/visor/ep.tga
	}
	{
		map models/players/visor/ep.tga
		blendfunc GL_ONE GL_ZERO
		alphaFunc GE128
		rgbGen entity
	}
}

models/players/xaero/ep
{
	nopicmip
	{
		map models/players/xaero/ep.tga
	}
	{
		map models/players/xaero/ep.tga
		blendfunc GL_ONE GL_ZERO
		alphaFunc GE128
		rgbGen entity
	}
}

models/players/xaero/ep_h
{
	nopicmip
	{
		map models/players/xaero/ep_h.tga
	}
	{
		map models/players/xaero/ep_h.tga
		blendfunc GL_ONE GL_ZERO
		alphaFunc GE128
		rgbGen entity
	}
}

models/players/xaero/xaero_a_ql
{
	nopicmip
    {
		map textures/effects/envmaprail.tga
        tcmod rotate 350
        tcmod scroll 3 1
        blendfunc GL_ONE GL_ZERO
		rgbGen identity
	} 
	{
		map models/players/xaero/xaero_ql_a.tga
		blendfunc blend
		rgbGen lightingDiffuse
	}
	{
		map models/players/xaero/brightglow2_a.tga
		blendfunc add
		rgbGen wave triangle 1 0 0 0
	}
	{
		map models/players/xaero/brightglow_a.tga
		blendfunc add
		rgbGen wave triangle 1 0.5 0.5 1
	}
}

models/players/xaero/xaero_a
{      
	nopicmip
    {
		map textures/effects/envmapbfg.tga
        tcmod rotate 350
        tcmod scroll 3 1
        blendFunc GL_ONE GL_ZERO
		rgbGen identity
	} 
    {
        map models/players/xaero/xaero_a.tga
		blendFunc blend
		rgbGen lightingdiffuse
	}
}

