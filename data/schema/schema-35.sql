ALTER TABLE %allsongstables ADD COLUMN dances TEXT DEFAULT "";
ALTER TABLE playlist_items ADD COLUMN dances TEXT DEFAULT "";

UPDATE schema_version SET version=35;
