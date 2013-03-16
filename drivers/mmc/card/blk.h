#ifndef MMC_BLK_H
#define MMC_BLK_H

struct gendisk;

#ifdef CONFIG_MMC_BLOCK_QUIRKS
struct mmc_blk_data;

#define mmc_blk_qrev(hwrev, fwrev, year, month) \
	(((u64) hwrev) << 40 |                  \
	 ((u64) fwrev) << 32 |                  \
	 ((u64) year) << 16 |                   \
	 ((u64) month))

#define mmc_blk_qrev_card(card)       \
	mmc_blk_qrev(card->cid.hwrev, \
		     card->cid.fwrev, \
		     card->cid.year,  \
		     card->cid.month)

#define MMC_BLK_QUIRK_VERSION(_name, _manfid, _oemid, _rev_start, _rev_end, _probe, _adjust) \
	{							\
		.name = (_name),				\
		.manfid = (_manfid),				\
		.oemid = (_oemid),				\
		.rev_start = (_rev_start),			\
		.rev_end = (_rev_end),				\
		.probe = (_probe),				\
		.adjust = (_adjust),				\
	}

#define MMC_BLK_QUIRK(_name, _manfid, _oemid, _probe, _adjust)	\
	MMC_BLK_QUIRK_VERSION(_name, _manfid, _oemid, 0, -1ull, _probe, _adjust)

extern struct mmc_blk_quirk *mmc_blk_quirk_find(struct mmc_card *card);
#else
static inline struct mmc_blk_quirk *mmc_blk_quirk_find(struct mmc_card *card)
{
	return NULL;
}
#endif /* CONFIG_MMC_BLOCK_QUIRKS */

struct mmc_blk_quirk {
	const char *name;

	/* First valid revision */
	u64 rev_start;

	/* Last valid revision */
	u64 rev_end;

	unsigned int manfid;
	unsigned short oemid;
	int (*probe)(struct mmc_blk_data *, struct mmc_card *);
	void (*adjust)(struct mmc_queue *, struct request *, struct mmc_request *);
};

/*
 * There is one mmc_blk_data per slot.
 */
struct mmc_blk_data {
	spinlock_t	lock;
	struct gendisk	*disk;
	struct mmc_queue queue;

	unsigned int	usage;
	unsigned int	read_only;
	unsigned int	write_align_size;
	unsigned int	write_align_limit;
	struct mmc_blk_quirk *quirk;
};

#endif
