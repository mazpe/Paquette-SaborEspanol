package Paquette;

use strict;
use warnings;

use Catalyst::Runtime 5.80;

# Set flags and add plugins for the application
#
#         -Debug: activates the debug mode for very useful log messages
#   ConfigLoader: will load the configuration from a Config::General file in the
#                 application's home directory
# Static::Simple: will serve static files from the application's root
#                 directory

use parent qw/Catalyst/;
use Catalyst qw/ 
                -Debug
                ConfigLoader
                Static::Simple
                StackTrace

                SubRequest
            
                Authentication

                Session
                Session::Store::File
                Session::State::Cookie
/;
our $VERSION = '0.05';

# Configure the application.
#
# Note that settings in paquette.conf (or other external
# configuration file that you set up manually) take precedence
# over this when using ConfigLoader. Thus configuration
# details given here can function as a default configuration,
# with an external configuration file acting as an override for
# local deployment.

__PACKAGE__->config( 
    name        => 'Paquette',
    session     => {flash_to_stash => 1},
    default_view    => 'TT',
    'View::Email::Template' => {
        default => {
            content_type => 'text/plain',
            charset => 'utf-8',
            view => 'TTEmail',
        },
        sender => {
            mailer => 'SMTP',
            mailer_args => {
                Host     => 'mail.server.com', # defaults to localhost
                username => 'username',
                password => 'password',
            }
        },
        template_prefix => 'email',
    }

);

#__PACKAGE__->config->{'Plugin::Authentication'} = {
#        default => {
#            class           => 'SimpleDB',
#            #user_model      => 'PaquetteDB::Customer',
#            user_class      => 'PaquetteDB::Customer',
#            password_type   => 'self_check',
#        },

#    };

__PACKAGE__->config(
    'Plugin::Authentication' => {
        default_realm => 'customers',
        realms => {
            customers => {
                credential => {
                    class          => 'Password',
                    password_field => 'password',
                    password_type  => 'self_check'
                },
                store => {
                    class => 'DBIx::Class',
                    user_model => 'PaquetteDB::Customer',
                }
            }
        }
    },
);

# Start the application
__PACKAGE__->setup();


=head1 NAME

Paquette - Catalyst based application

=head1 SYNOPSIS

    script/paquette_server.pl

=head1 DESCRIPTION

[enter your description here]

=head1 SEE ALSO

L<Paquette::Controller::Root>, L<Catalyst>

=head1 AUTHOR

root

=head1 LICENSE

This library is free software. You can redistribute it and/or modify
it under the same terms as Perl itself.

=cut

1;
