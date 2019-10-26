-- ----------------------------
-- Table structure for tb_test
-- ----------------------------
DROP TABLE IF EXISTS "public"."tb_test";
CREATE TABLE "public"."tb_test" (
"id" serial NOT NULL,
"name" text  COLLATE "default" NOT NULL
);
ALTER TABLE "public"."tb_test" ADD PRIMARY KEY ("id");
-- ----------------------------
-- Records of tb_test
-- ----------------------------
--INSERT INTO "public"."tb_test" VALUES ('1', 'name1');
--INSERT INTO "public"."tb_test" VALUES ('2', 'name2');


-- ----------------------------
-- Table structure for tb_sum
-- ----------------------------
DROP TABLE IF EXISTS "public"."tb_sum";
CREATE TABLE "public"."tb_sum" (
"id" int4 NOT NULL,
"name" text  COLLATE "default" NOT NULL,
"sum" int4 NOT NULL
);
ALTER TABLE "public"."tb_sum" ADD PRIMARY KEY ("id");
-- ----------------------------
-- Records of tb_sum
-- ----------------------------
INSERT INTO "public"."tb_sum" VALUES ('1', 'name1','1');
INSERT INTO "public"."tb_sum" VALUES ('2', 'name2','2');

