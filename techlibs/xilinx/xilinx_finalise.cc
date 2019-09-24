/*
 *  yosys -- Yosys Open SYnthesis Suite
 *
 *  Copyright (C) 2012  Clifford Wolf <clifford@clifford.at>
 *            (C) 2019  Eddie Hung    <eddie@fpgeh.com>
 *
 *  Permission to use, copy, modify, and/or distribute this software for any
 *  purpose with or without fee is hereby granted, provided that the above
 *  copyright notice and this permission notice appear in all copies.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include "kernel/register.h"
#include "kernel/celltypes.h"
#include "kernel/rtlil.h"
#include "kernel/log.h"

USING_YOSYS_NAMESPACE
PRIVATE_NAMESPACE_BEGIN

struct XilinxFinalisePass : public Pass
{
	XilinxFinalisePass() : Pass("xilinx_finalise", "") { }

	void help() YS_OVERRIDE
	{
		//   |---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|
		log("\n");
		log("    xilinx_finalise [options]\n");
		log("\n");
	}

	void execute(std::vector<std::string> args, RTLIL::Design *design) YS_OVERRIDE
	{
		size_t argidx;
		for (argidx = 1; argidx < args.size(); argidx++)
		{
			break;
		}
		extra_args(args, argidx, design);

		log_header(design, "Executing XILINX_FINALISE pass.\n");

		for (auto module : design->selected_modules())
		for (auto cell : module->selected_cells()) {
			if (cell->type != ID(DSP48E1))
				continue;
			for (auto &conn : cell->connections_) {
				if (!cell->output(conn.first))
					continue;
				bool purge = true;
				for (auto &chunk : conn.second.chunks()) {
					auto it = chunk.wire->attributes.find(ID(unused_bits));
					if (it == chunk.wire->attributes.end())
						continue;

					std::string unused_bits = stringf("%d", chunk.offset);
					for (auto i = 1; i < chunk.width; i++)
						unused_bits += stringf(" %d", i+chunk.offset);

					if (it->second.decode_string().find(unused_bits) == std::string::npos) {
						purge = false;
						break;
					}
				}

				if (purge) {
					log_debug("Purging unused port connection %s %s (.%s(%s))\n", cell->type.c_str(), log_id(cell), log_id(conn.first), log_signal(conn.second));
					conn.second = SigSpec();
				}
			}
		}
	}
} XilinxFinalisePass;

PRIVATE_NAMESPACE_END
