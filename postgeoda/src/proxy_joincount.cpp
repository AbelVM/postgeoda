/**
 * Author: Xun Li <lixun910@gmail.com>
 *
 * Changes:
 * 2021-1-27 Update to use libgeoda 0.0.6; add pg_local_joincount()
 */

#include <vector>

#include <libgeoda/gda_sa.h>
#include <libgeoda/sa/LISA.h>
#include <libgeoda/weights/GalWeight.h>
#include <libgeoda/GeoDaSet.h>
#include <libgeoda/GenUtils.h>
#include <libgeoda/pg/geoms.h>
#include <libgeoda/pg/utils.h>

#include "binweight.h"
#include "postgeoda.h"
#include "proxy.h"

Point** pg_local_joincount(int N, const int64* fids, const double* r, const uint8_t* bw)
{
    BinWeight* w = new BinWeight(bw); // complete weights
    int64 num_obs  = w->num_obs;

    // NOTE: num_obs could be larger than N
    // input values could be a subset of total values
    std::vector<double> data(num_obs +1 , 0);
    std::vector<bool> undefs(num_obs +1, true);

    for (size_t i=0; i<N; ++i) {
        size_t fid = fids[i];
        if (fid > num_obs) {
            lwerror("pg_local_joincount: fid not match in weights. fid > w.num_obs %d", fids[i]) ;
        }
        data[fid] = r[i];
        undefs[fid] = false;
        //lwdebug(1, "pg_local_joincount: data[%d] = %f, nn=%d", fid, data[fid], w->GetNbrSize(fid));
    }

    lwdebug(1, "pg_local_joincount: gda_localjoincount()");
    double significance_cutoff = 0.05;
    int nCPUs = 8, permutations = 999, last_seed_used = 123456789;
    std::string perm_method = "complete";

    LISA* lisa = gda_localjoincount(w, data, undefs, significance_cutoff, nCPUs, permutations, perm_method, last_seed_used);
    // report: cluster indicator and p-value
    const std::vector<int>& lisa_c = lisa->GetClusterIndicators();
    const std::vector<double>& lisa_p = lisa->GetLocalSignificanceValues();

    // results
    Point **result = (Point **) palloc(sizeof(Point*) * N);
    for (size_t i = 0; i < N; i++) {
        result[i] = (Point *) palloc(sizeof(Point));
        result[i]->x = lisa_c[ fids[i] ];
        result[i]->y = lisa_p[ fids[i] ];
    }

    // clean
    delete lisa;
    delete w;
    lwdebug(1, "Exit pg_local_joincount.");
    return result;
}


Point** pg_bivariate_local_joincount(int N, const int64* fids, const double* r1, const double* r2, const uint8_t* bw)
{
    BinWeight* w = new BinWeight(bw); // complete weights
    int64 num_obs  = w->num_obs;

    // NOTE: num_obs could be larger than N
    // input values could be a subset of total values
    std::vector<double> data1(num_obs +1 , 0);
    std::vector<double> data2(num_obs +1 , 0);
    std::vector<bool> undefs1(num_obs +1, false);
    std::vector<bool> undefs2(num_obs +1, false);

    for (size_t i=0; i<N; ++i) {
        size_t fid = fids[i];
        if (fid > num_obs) {
            lwerror("pg_bivariate_local_joincount: fid not match in weights. fid > w.num_obs %d", fids[i]) ;
        }
        data1[fid] = r1[i];
        data2[fid] = r2[i];
        //lwdebug(1, "pg_bivariate_local_joincount: data[%d] = %f, nn=%d", fid, data[fid], w->GetNbrSize(fid));
    }

    std::vector<std::vector<double> > data;
    data.push_back(data1);
    data.push_back(data2);

    std::vector<std::vector<bool> > undefs;
    undefs.push_back(undefs1);
    undefs.push_back(undefs2);

    lwdebug(1, "pg_bivariate_local_joincount: pg_bivariate_local_joincount()");
    double significance_cutoff = 0.05;
    int nCPUs = 8, permutations = 999, last_seed_used = 123456789;
    std::string perm_method = "complete";

    LISA* lisa = gda_localmultijoincount(w, data, undefs, significance_cutoff, nCPUs, permutations, perm_method, last_seed_used);
    // report: cluster indicator and p-value
    const std::vector<int>& lisa_c = lisa->GetClusterIndicators();
    const std::vector<double>& lisa_p = lisa->GetLocalSignificanceValues();

    // results
    Point **result = (Point **) palloc(sizeof(Point*) * N);
    for (size_t i = 0; i < N; i++) {
        result[i] = (Point *) palloc(sizeof(Point));
        result[i]->x = lisa_c[ fids[i] ];
        result[i]->y = lisa_p[ fids[i] ];
    }

    // clean
    delete lisa;
    delete w;
    lwdebug(1, "Exit pg_bivariate_local_joincount.");
    return result;
}