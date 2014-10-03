package Paquette::Controller::Admin::Promotions;

use Moose;
BEGIN { extends 'Catalyst::Controller' }
use Paquette::Form::Admin::Promotion;
use Data::Dumper;

has 'promotion_form' => (
    isa => 'Paquette::Form::Admin::Promotion',
    is => 'ro',
    lazy => 1,
    default => sub { Paquette::Form::Admin::Promotion->new },
);


=head1 NAME

Paquette::Controller::Admin::Promotions - Catalyst Controller

=head1 DESCRIPTION

Catalyst Controller.

=head1 METHODS

=cut


sub index :Chained('base') :Path :Args(0) {
    my ( $self, $c ) = @_;

    my $promotions = [$c->model('PaquetteDB::Promotion')->all];

    $c->stash->{'promotions'}    = $promotions;
    $c->stash->{wrapper_admin}  = "1";
    $c->stash->{template} = 'admin/promotions_list.tt2';

    return;
}

sub base :Chained('/') :PathPart('admin/promotions') :CaptureArgs(0) {
    my ( $self, $c ) = @_;

    $c->stash->{wrapper_admin}  = "1";
}

sub load : Chained('base') :PathPart('') :CaptureArgs(1) {
    my ( $self, $c, $id ) = @_;
    my $promotion;

    if($id) {

        $c->stash->{promotion_id} = $id;
        $promotion    = $c->model('PaquetteDB::Promotion')->find($id);

    } else {

        $c->response->status(404);
        $c->detach;

    }

    if ($promotion) {

        $c->stash->{'promotion'} = $promotion;

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
    $row = $c->model('PaquetteDB::Promotion')->new_result({});

    # Set our template and form to use
    $c->stash(
        template    => 'admin/promotion.tt2',
        form        => $self->promotion_form,
    );

    # Process our form
    $form =  $self->promotion_form->process (
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
    my $promotion_id;
    my $form;
    my $row;

    $promotion_id = $c->stash->{promotion_id};

    # Get a new empty row
    #$row = $c->model('PaquetteDB::Promotion')->get_promotion($promotion_id);
    $row = $c->stash->{promotion};

    # Set our template and form to use
    $c->stash(
        template    => 'admin/promotion.tt2',
        form        => $self->promotion_form,
    );

    # Process our form
    $form =  $self->promotion_form->process (
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

    my $promotion = $c->stash->{promotion};

    if ($promotion) {

        $promotion->delete;

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
