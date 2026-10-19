create table public.users (
  id bigint not null,
  created_at timestamp with time zone not null default now(),
  wins bigint null default '0'::bigint,
  loses bigint null default '0'::bigint,
  rankpoints bigint null default '100'::bigint,
  levelpoints bigint null default '0'::bigint,
  level bigint null default '1'::bigint,
  "nextLevelPoints" bigint null default '1000'::bigint,
  "nextRankPoints" bigint null default '250'::bigint,
  constraint users_pkey primary key (id)
) TABLESPACE pg_default;