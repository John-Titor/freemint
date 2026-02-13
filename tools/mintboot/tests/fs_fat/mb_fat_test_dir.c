#include "mb_fat_tests_internal.h"

void mb_fat_tests_phase_dir_integrity(struct mb_fat_test_ctx *t)
{
	long rc;

	rc = Dcreate(t->dcreate_missing_parent);
	if (rc != MB_ERR_PTHNF)
		mb_panic("FAT test: Dcreate missing parent rc=%d expected %d",
			 (int)rc, MB_ERR_PTHNF);

	rc = Dcreate(t->dcreate_root);
	if (rc != 0)
		mb_panic("FAT test: Dcreate rc=%d expected 0", (int)rc);

	rc = Dcreate(t->dcreate_exist);
	if (rc != MB_ERR_ACCDN)
		mb_panic("FAT test: Dcreate exists rc=%d expected %d", (int)rc,
			 MB_ERR_ACCDN);

	rc = Ddelete(t->ddelete_dir);
	if (rc != 0)
		mb_panic("FAT test: Ddelete dir rc=%d expected 0", (int)rc);
	rc = Ddelete(t->ddelete_missing);
	if (rc != MB_ERR_FILNF)
		mb_panic("FAT test: Ddelete missing rc=%d expected %d", (int)rc,
			 MB_ERR_FILNF);
	rc = Ddelete(t->ddelete_file);
	if (rc != MB_ERR_ACCDN)
		mb_panic("FAT test: Ddelete file rc=%d expected %d", (int)rc,
			 MB_ERR_ACCDN);
	rc = Ddelete(t->ddelete_missing_dir);
	if (rc != MB_ERR_FILNF)
		mb_panic("FAT test: Ddelete missing dir rc=%d expected %d",
			 (int)rc, MB_ERR_FILNF);
	rc = Ddelete(t->ddelete_nonempty);
	if (rc != MB_ERR_ACCDN)
		mb_panic("FAT test: Ddelete nonempty rc=%d expected %d",
			 (int)rc, MB_ERR_ACCDN);

	rc = mb_fat_check(mb_tests_drive_dev, &t->report);
	if (rc != 0)
		mb_panic("FAT test: integrity rc=%d bad_bpb=%u bad_dirent=%u bad_chain=%u bad_fat=%u lost=%u cross=%u",
			 (int)rc, t->report.bad_bpb, t->report.bad_dirent,
			 t->report.bad_chain, t->report.bad_fat, t->report.lost_clusters,
			 t->report.cross_links);
	if (t->report.bad_bpb || t->report.bad_dirent || t->report.bad_chain ||
	    t->report.bad_fat || t->report.lost_clusters || t->report.cross_links)
		mb_panic("FAT test: integrity bad_bpb=%u bad_dirent=%u bad_chain=%u bad_fat=%u lost=%u cross=%u",
			 t->report.bad_bpb, t->report.bad_dirent, t->report.bad_chain,
			 t->report.bad_fat, t->report.lost_clusters, t->report.cross_links);
}
