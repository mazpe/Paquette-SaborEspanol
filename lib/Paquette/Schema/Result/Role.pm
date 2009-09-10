package Paquette::Schema::Result::Role;

use strict;
use warnings;

use base 'DBIx::Class';

__PACKAGE__->load_components("InflateColumn::DateTime", "TimeStamp", "EncodedColumn", "Core");
__PACKAGE__->table("role");
__PACKAGE__->add_columns(
  "id",
  { data_type => "INT", default_value => undef, is_nullable => 0, size => 11 },
  "role",
  {
    data_type => "VARCHAR",
    default_value => undef,
    is_nullable => 1,
    size => 255,
  },
);
__PACKAGE__->set_primary_key("id");


# Created by DBIx::Class::Schema::Loader v0.04005 @ 2009-09-10 08:14:55
# DO NOT MODIFY THIS OR ANYTHING ABOVE! md5sum:JdlJkvuKn+aXpDlgewM92Q

__PACKAGE__->has_many(map_user_roles => 
    'Paquette::Schema::Result::UserRole', 
    'role_id'
);

# You can replace this text with custom content, and it will be preserved on regeneration
1;
