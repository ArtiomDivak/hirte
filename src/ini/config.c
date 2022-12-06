#include "config.h"
#include "ini.h"
#include <stdlib.h>


int match(const char *section, const char *check_section, const char *name, const char *check_name) {
        if (strcmp(section, check_section) == 0 && strcmp(name, check_name) == 0) {
                return EXIT_SUCCESS;
        }
        return EXIT_FAILURE;
}

int key_value_compare_key(const void *a, const void *b, void *udata) {
        const keyValue *kva = a;
        const keyValue *kvb = b;
        if (kva->value == NULL || kvb->value == NULL) {
                return strcmp(kva->key, kvb->key);
        }
        return strcmp(kva->key, kvb->key) && strcmp(kva->value, kvb->value);
}

uint64_t key_value_hash(const void *item, uint64_t seed0, uint64_t seed1) {
        const keyValue *kv = item;
        return hashmap_sip(kv->key, strlen(kv->key), seed0, seed1);
}

int topic_compare_topic_name(const void *a, const void *b, void *udata) {
        const topic *ta = a;
        const topic *tb = b;
        return strcmp(ta->topic, tb->topic);
}

uint64_t topic_hash(const void *item, uint64_t seed0, uint64_t seed1) {
        const topic *top = item;
        return hashmap_sip(top->topic, strlen(top->topic), seed0, seed1);
}

/*
This function will be used in ini_parse() for node ini file parsing.
*/
static int handler(void *user, const char *section, const char *name, const char *value) {
        assert(section != NULL);
        assert(name != NULL);
        assert(user != NULL);
        assert(value != NULL);

        char *sec = strdup(section);
        char *name_dup = strdup(name);
        char *value_dup = strdup(value);

        struct hashmap *topics = (struct hashmap *) user;
        keyValue *key_value = (keyValue *) malloc(sizeof(keyValue));
        key_value->key = name_dup;
        key_value->value = value_dup;
        topic *tmptopic = hashmap_get((struct hashmap *) user, &(topic){ .topic = sec });
        if (tmptopic != NULL) {
                hashmap_set(tmptopic->keys_and_values, key_value);
                free(sec);
        } else {
                struct hashmap *hm = hashmap_new(
                                sizeof(keyValue), 0, 0, 0, key_value_hash, key_value_compare_key, NULL, NULL);
                hashmap_set(hm, key_value);
                tmptopic = (topic *) malloc(sizeof(topic));
                tmptopic->keys_and_values = hm;
                tmptopic->topic = sec;
                hashmap_set(topics, tmptopic);
                free(tmptopic);
        }
        free(key_value);
        return EXIT_SUCCESS;
}

/* Get the file path
    if it can't find the or cant parse the path it will return configurationOrch
    with nulls else will return configurationOrch with all the data */
struct hashmap *parsing_ini_file(const char *file) {
        if (file == NULL) {
                return NULL;
        }
        struct hashmap *temp = hashmap_new(
                        sizeof(topic), 0, 0, 0, topic_hash, topic_compare_topic_name, NULL, NULL);
        if (ini_parse(file, handler, temp) < 0) {
                printf("Can't load the file %s\n", file);
                return NULL;
        }
        return temp;
}

void *print_all_topics(struct hashmap *topics) {
        size_t iter = 0;
        void *item;
        while (hashmap_iter(topics, &iter, &item)) {
                topic *tp = item;
                size_t iter_kv = 0;
                void *item_kv;
                printf("[%s]\n", tp->topic);
                while (hashmap_iter(tp->keys_and_values, &iter_kv, &item_kv)) {
                        keyValue *kv = item_kv;
                        printf("\t%s :", kv->key);
                        printf(" %s\n", kv->value);
                }
        }
}

void free_topics_hashmap(struct hashmap **topics) {
        size_t iter = 0;
        void *item;
        while (hashmap_iter(*topics, &iter, &item)) {
                topic *tp = item;
                size_t iter_kv = 0;
                void *item_kv;
                int len = hashmap_count(tp->keys_and_values);
                keyValue *free_kv[len];
                int i = 0;
                while (hashmap_iter(tp->keys_and_values, &iter_kv, &item_kv)) {
                        keyValue *kv = (keyValue *) item_kv;
                        free(kv->value);
                        free(kv->key);
                }
                free(tp->topic);
                hashmap_free(tp->keys_and_values);
        }
        hashmap_free(*topics);
}


/*example of how to use parsing_ini_file
void main() {
        const char *file = "../../doc/example.ini";

        struct hashmap *topics _cleanup_hashmap_ = parsing_ini_file(file);
        print_all_topics(topics);

        printf("Done\n");
}*/