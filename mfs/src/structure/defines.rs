pub const STAT_VFS_SIZE:usize = 112;
pub const STAT_SIZE:usize = 144;
pub const NODE_SIZE:usize = 4096;
pub const NODE_FOR_RECOVERY:usize = 100; //最后一部分dnode用于log和恢复，不存储
pub const NODE_NUM_TOTAL:usize = 16384;