#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sphinxbase/err.h>
#include <pocketsphinx.h>

int speechToText(char text[])
{
    ps_decoder_t *ps;
    cmd_ln_t *config;
    FILE *fh;
    char const *hyp, *uttid;
    int16 buf[512];
    int rv;
    int32 score;

    config = cmd_ln_init(NULL, ps_args(), TRUE,
			 "-hmm","./language-model/",
			 "-lm", "./language-dic/2195.lm",
			 "-dict", "./language-dic/2195.dic",
			 "-logfn", "/dev/null",
			 NULL);

    /* err_set_logfile("/log/sphinx"); */

    
    if (config == NULL) {
        fprintf(stderr, "Failed to create config object, see log for details\n");
        return -1;
    }
    
    ps = ps_init(config);
    if (ps == NULL) {
        fprintf(stderr, "Failed to create recognizer, see log for details\n");
        return -1;
    }

    fh = fopen("recorded.raw", "rb");
    if (fh == NULL) {
        fprintf(stderr, "Unable to open input file\n");
        return -1;
    }

    rv = ps_start_utt(ps);
    
    while (!feof(fh)) {
        size_t nsamp;
        nsamp = fread(buf, 2, 512, fh);
        rv = ps_process_raw(ps, buf, nsamp, FALSE, FALSE);
    }
    
    rv = ps_end_utt(ps);
    hyp = ps_get_hyp(ps, &score);
    strcpy(text, hyp);

    fclose(fh);
    ps_free(ps);
    cmd_ln_free_r(config);
    
    return 0;
}
