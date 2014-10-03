package Paquette::Controller::Admin::Countries;

use Moose;
BEGIN { extends 'Catalyst::Controller' }
use Paquette::Form::Admin::Country;
use Data::Dumper;

has 'country_form' => (
    isa => 'Paquette::Form::Admin::Country',
    is => 'ro',
    lazy => 1,
    default => sub { Paquette::Form::Admin::Country->new },
);


=head1 NAME

Paquette::Controller::Admin::Countries - Catalyst Controller

=head1 DESCRIPTION

Catalyst Controller.

=head1 METHODS

=cut


sub index :Chained('base') :Path :Args(0) {
    my ( $self, $c ) = @_;

    my $countries = [$c->model('PaquetteDB::Countries')->all];

    $c->stash->{'countries'}    = $countries;
    $c->stash->{wrapper_admin}  = "1";
    $c->stash->{template} = 'admin/countries_list.tt2';

    return;
}

sub base :Chained('/') :PathPart('admin/countries') :CaptureArgs(0) {
    my ( $self, $c ) = @_;

    $c->stash->{wrapper_admin}  = "1";
}

sub load : Chained('base') :PathPart('') :CaptureArgs(1) {
    my ( $self, $c, $id ) = @_;
    my $country;

    if($id) {

        $c->stash->{country_id} = $id;
        $country    = $c->model('PaquetteDB::Countries')->find($id);

    } else {

        $c->response->status(404);
        $c->detach;

    }

    if ($country) {

        $c->stash->{'country'} = $country;

    } else {

        $c->response->status(404);
        $c->detach;

    }

    return;
}

sub creation : Local {
    my ( $self, $c ) = @_;
    my $row;
    my $form;

$c->stash->{wrapper_admin}  = "1";

    # Get a new empty row
    $row = $c->model('PaquetteDB::Countries')->new_result({});

    # Set our template and form to use
    $c->stash(
        template    => 'admin/country.tt2',
        form        => $self->country_form,
    );

    # Process our form
    $form =  $self->country_form->process (
        item            => $row,
        params          => $c->req->params,
    );

    # If the form has been submited sucessfully, then redirect to confirm page
    if ($form) {
        $c->res->redirect( $c->uri_for($self->action_for('index')) );
    }

}

sub edit : Chained('load') : PathPart('edit') : Args(0) {
    my ( $self, $c ) = @_;
    my $country_id;
    my $form;
    my $row;

    $country_id = $c->stash->{country_id};

    # Get a new empty row
    #$row = $c->model('PaquetteDB::Countries')->get_country($country_id);
    $row = $c->stash->{country};

    # Set our template and form to use
    $c->stash(
        template    => 'admin/country.tt2',
        form        => $self->country_form,
    );

    # Process our form
    $form =  $self->country_form->process (
        item            => $row,
        params          => $c->req->params,
    );

    # If the form has been submited sucessfully, then redirect to confirm page
    if ($form) {
        $c->res->redirect( $c->uri_for($self->action_for('index')) );
    }

}

sub delete : Chained('load') : PathPart('delete') : Args(0) {
    my ( $self, $c ) = @_;

    my $country = $c->stash->{country};

    if ($country) {

        $country->delete;

        $c->response->redirect(
            $c->uri_for( $self->action_for('index')) . '/' );

    } else {

        $c->response->status(404);
        $c->detach;

    }

    return;
}


=head1 AUTHOR

Lester Ariel Mesa,,lesterm@gmail.com,

=head1 LICENSE

This library is free software. You can redistribute it and/or modify
it under the same terms as Perl itself.

=cut

1;
