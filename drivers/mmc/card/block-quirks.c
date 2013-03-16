/*
 * linux/drivers/mmc/card/block-quirks.c
 *
 * Copyright (C) 2010 Andrei Warkentin <andreiw@xxxxxxxxxxxx>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include <linux/kernel.h>
#include <linux/blkdev.h>
#include <linux/semaphore.h>
#include <linux/mmc/card.h>
#include <linux/mmc/mmc.h>

#include "queue.h"
#include "blk.h"

#ifdef CONFIG_MMC_BLOCK_QUIRK_TOSHIBA_32NM
static int toshiba_32nm_probe(struct mmc_blk_data *md, struct mmc_card *card)
{
	printk(KERN_INFO "Applying Toshiba 32nm workarounds\n");
#ifdef CONFIG_MMC_BLOCK_QUIRK_TOSHIBA_32NM_REL
	printk(KERN_INFO "Toshiba 32nm reliability splits over 0x%x-0x%x blocks\n",
	       CONFIG_MMC_BLOCK_QUIRK_TOSHIBA_32NM_REL_L,
	       CONFIG_MMC_BLOCK_QUIRK_TOSHIBA_32NM_REL_U);
#endif

	/* Page size 8K, this card doesn't like unaligned writes
	   across 8K boundary. */
	md->write_align_size = 8192;

	/* Doing the alignment for accesses > 12K seems to
	   result in decreased perf. */
	md->write_align_limit = 12288;
	return 0;
}

#ifdef CONFIG_MMC_BLOCK_QUIRK_TOSHIBA_32NM_REL
static void toshiba_32nm_adjust(struct mmc_queue *mq,
				struct request *req,
				struct mmc_request *mrq)
{

	int err;
	struct mmc_command cmd;
	struct mmc_blk_data *md = mq->data;
	struct mmc_card *card = md->queue.card;

	if (rq_data_dir(req) != WRITE)
		return;

	if (blk_rq_sectors(req) > CONFIG_MMC_BLOCK_QUIRK_TOSHIBA_32NM_REL_U ||
	    blk_rq_sectors(req) < CONFIG_MMC_BLOCK_QUIRK_TOSHIBA_32NM_REL_L)
		return;

	/* 8K chunks */
	if (mrq->data->blocks > 16)
		mrq->data->blocks = 16;

	/*
	  We know what the valid values for this card are,
	  no need to check EXT_CSD_REL_WR_SEC_C.
	 */
	cmd.opcode = MMC_SET_BLOCK_COUNT | (1 << 31);
	cmd.arg = mrq->data->blocks;
	cmd.flags = MMC_RSP_R1 | MMC_CMD_AC;
	err = mmc_wait_for_cmd(card->host, &cmd, 0);
	if (!err)
		mrq->stop = NULL;
}
#else
#define toshiba_32nm_adjust NULL
#endif /* CONFIG_MMC_BLOCK_QUIRK_TOSHIBA_32NM_REL */
#endif /* CONFIG_MMC_BLOCK_QUIRK_TOSHIBA_32NM */

/*
   Caveat: Because this list is just looked up with a linear
   search, take care that either overlapping revision ranges
   don't exist, or that returning the first quirk that matches
   a given revision has the desired effect.
*/
struct mmc_blk_quirk mmc_blk_quirks[] = {
#ifdef CONFIG_MMC_BLOCK_QUIRK_TOSHIBA_32NM
        MMC_BLK_QUIRK("MMC16G", 0x11, 0x0,
		      toshiba_32nm_probe, toshiba_32nm_adjust),
        MMC_BLK_QUIRK("MMC32G", 0x11, 0x0100,
		      toshiba_32nm_probe, toshiba_32nm_adjust),
#endif /* CONFIG_MMC_BLOCK_QUIRK_TOSHIBA_32NM */
};

static int mmc_blk_quirk_cmp(const struct mmc_blk_quirk *quirk,
			      const struct mmc_card *card)
{
	u64 rev = mmc_blk_qrev_card(card);

	if (quirk->manfid == card->cid.manfid &&
	    quirk->oemid == card->cid.oemid &&
	    !strcmp(quirk->name, card->cid.prod_name) &&
	    rev >= quirk->rev_start &&
	    rev <= quirk->rev_end)
		return 0;

	return -1;
}

struct mmc_blk_quirk *mmc_blk_quirk_find(struct mmc_card *card)
{
	unsigned index;

	for (index = 0; index < ARRAY_SIZE(mmc_blk_quirks); index++)
		if (!mmc_blk_quirk_cmp(&mmc_blk_quirks[index], card))
			return &mmc_blk_quirks[index];

	return NULL;
}

