local process = require("wf.api.v1.process")
local superfamiconv = require("wf.api.v1.process.tools.superfamiconv")
local zx0 = require("wf.api.v1.process.tools.zx0")

local tileset = superfamiconv.convert_tileset(
	"wsx_console_font_default.png",
	superfamiconv.config()
		:mode("ws"):bpp(2)
		:color_zero("#ffffff")
		:no_discard():no_flip()
)

process.emit_symbol("wsx_console_font_default", zx0.compress(tileset.tiles))
