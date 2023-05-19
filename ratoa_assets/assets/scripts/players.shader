models/players/anarki/pm
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

models/players/anarki/pm_b
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

models/players/anarki/pm_x
{
	nopicmip
    {
      	map models/players/anarki/ep_x.tga
        blendfunc GL_ONE GL_ZERO
		alphaFunc GE128
		rgbGen identity
    }
}

models/players/anarki/pm_h
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

models/players/biker/pm
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

models/players/biker/pm_h
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

models/players/bitterman/pm
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

models/players/bitterman/pm_h
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

models/players/bones/pm
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

models/players/crash/pm
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

models/players/crash/pm_t
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

models/players/crash/pm_f
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

models/players/doom/pm
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

models/players/doom/pm_f
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

models/players/grunt/pm
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

models/players/grunt/pm_h
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

models/players/hunter/pm
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

models/players/hunter/pm_f
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

models/players/hunter/pm_h
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

models/players/keel/pm
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

models/players/keel/pm_h
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

models/players/klesk/pm
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

models/players/klesk/pm_h
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

models/players/lucy/pm
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

models/players/lucy/pm_h
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

models/players/major/pm
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

models/players/major/pm_h
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

models/players/mynx/pm_t
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

models/players/orbb/pm
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

models/players/orbb/pm_h
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

models/players/ranger/pm
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

models/players/razor/pm
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

models/players/razor/pm_h
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

models/players/sarge/pm
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

models/players/slash/pm
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

models/players/slash/pm_h
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

models/players/sorlag/pm
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

models/players/sorlag/pm_t
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

models/players/tankjr/pm
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

models/players/uriel/pm
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

models/players/uriel/pm_h
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

models/players/uriel/pm_w
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

models/players/visor/pm
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

models/players/xaero/pm
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

models/players/xaero/pm_h
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

